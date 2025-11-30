#include "vrpch.h"
#include "Volt-Renderer/GlobalIllumination/GlobalIlluminationRenderer.h"
#include "Volt-Renderer/RenderingTechniques/CascadedShadowMapsTechnique.h"
#include "Volt-Renderer/RenderView.h"
#include "Volt-Renderer/GPUScene.h"
#include "Volt-Renderer/BlueNoise.h"
#include "Volt-Renderer/SceneRendererRenderGraphData.h"
#include "Volt-Renderer/RenderScene.h"
#include "Volt-Renderer/RayTracing/RayTracingScene.h"
#include "Volt-Renderer/Renderer.h"
#include "Volt-Renderer/Camera/Camera.h"
#include "Volt-Renderer/ShapeLibrary.h"
#include "Volt-Renderer/SystemTextures.h"

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <RenderCore/RenderGraph/ShaderRegistry.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderContext.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/DefaultShaders.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/PipelineStateCache.h>
#include <RenderCore/SamplerStateCache.h>

namespace Volt
{
	static ConsoleVariable<int32_t> s_giSpatialHashTableSize(
		"r.GI.SpatialHashTable.Size",
		500'000,
		"The size of the spatial hash table."
	);

	static ConsoleVariable<float> s_giWorldRadianceCacheLodDistance(
		"r.GI.WorldRadianceCache.LodDistance",
		5000,
		"The size of a lod level in the world radiance cache."
	);

	static ConsoleVariable<float> s_giWorldRadianceCacheCellSize(
		"r.GI.WorldRadianceCache.CellSize",
		25,
		"The base size of a cell in the world radiance cache."
	);

	static ConsoleVariable<int32_t> s_giWorldRadianceCacheCellLifetime(
		"r.GI.WorldRadianceCache.CellLifetime",
		5,
		"The number of frames a cell is valid."
	);

	static ConsoleVariable<int32_t> s_giVisualizeWorldRadianceCache(
		"r.GI.Visualize.WorldRadianceCache",
		0,
		"Whether or not to visualize the world radiance cache"
	);

	static ConsoleVariable<int32_t> s_giVisualizeIrradianceVolume(
		"r.GI.Visualize.IrradianceVolume",
		0,
		"Whether or not to visualize the irradiance volume"
	);

	static ConsoleVariable<float> s_giIrradianceVolumeSpacing(
		"r.GI.IrradianceVolume.Spacing",
		50,
		"The base spacing of the probes in the irradiance volume."
	);

	static ConsoleVariable<int32_t> s_giIrradianceVolumeResolution(
		"r.GI.IrradianceVolume.Resolution",
		16,
		"The resolution of a cascade in probes."
	);

	static ConsoleVariable<int32_t> s_giIrradianceVolumeProbeResolution(
		"r.GI.IrradianceVolume.ProbeResolution",
		8,
		"The resolution of probe in the probe atlas (without borders)."
	);

	static ConsoleVariable<int32_t> s_giIrradianceVolumeNumCascades(
		"r.GI.IrradianceVolume.NumCascades",
		6,
		"The number of cascades in the irradiance volume."
	);

	static ConsoleVariable<int32_t> s_giIrradianceVolumeFreezeAtWorldOrigin(
		"r.GI.IrradianceVolume.FreezeAtWorldOrigin",
		1,
		"Whether or not to freeze the irradiance volume at world origin."
	);

	struct FinalGatherCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(FinalGatherCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float4>, RWIndirectLight)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, GBufferAlbedo)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, GBufferNormal)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float2>, GBufferMaterial)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float>, SceneDepth)
			SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<uint>, WorldRadianceCacheCellCache)
			SHADER_PARAMETER_ACCELERATION_STRUCTURE(TLAS)
			SHADER_PARAMETER_RAY_TRACING_RESOURCE_TABLE(RTResources)
			SHADER_PARAMETER_STRUCT_INCLUDE(GPUSceneParameters, GPUScene)
			SHADER_PARAMETER_STRUCT_INCLUDE(BlueNoiseShaderParameters, BlueNoise)
			SHADER_PARAMETER_STRUCT_INCLUDE(SpatialHashTableParameters, SpatialHashTableParams)
			SHADER_PARAMETER_STRUCT_INCLUDE(WorldRadianceCacheParameters, WRCParameters)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(FinalGatherCS, "Engine/Shaders/Source/GlobalIllumination/FinalGather.hlsl", "FinalGatherCS", Compute);

	struct TemporalAccumulationCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(TemporalAccumulationCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float4>, RWIndirectLight)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float4>, RWPrevFrameIndirectLight)
			SHADER_PARAMETER(float, AccumulationAlpha)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(TemporalAccumulationCS, "Engine/Shaders/Source/GlobalIllumination/TemporalAccumulation.hlsl", "TemporalAccumulationCS", Compute);
	
	struct WorldRadianceCacheUpdateCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(WorldRadianceCacheUpdateCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER_UAV(RWStructuredBuffer<uint>, RWWorldRadianceCacheCellCache)
			SHADER_PARAMETER_BUFFER_UAV(RWStructuredBuffer<uint>, RWWorldRadianceCacheCellInfo)
			SHADER_PARAMETER_STRUCT_INCLUDE(SpatialHashTableParameters, SpatialHashTableParams)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(WorldRadianceCacheUpdateCS, "Engine/Shaders/Source/GlobalIllumination/WorldRadianceCacheUpdateCS.hlsl", "WorldRadianceCacheUpdateCS", Compute);

	struct TraceIrradianceVolumeCascadeCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(TraceIrradianceVolumeCascadeCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
			SHADER_PARAMETER_BUFFER_UAV(RWByteAddressBuffer, RWRayInfo)
			SHADER_PARAMETER_BUFFER_UAV(RWStructuredBuffer<uint>, RWWorldRadianceCacheCellsToShade)
			SHADER_PARAMETER_BUFFER_UAV(RWStructuredBuffer<uint2>, RWWorldRadianceCacheCellShadingInfo)
			SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWShadingIndirectArgs)
			SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWWorldRadianceCacheCellMark)
			SHADER_PARAMETER_ACCELERATION_STRUCTURE(TLAS)
			SHADER_PARAMETER_RAY_TRACING_RESOURCE_TABLE(RTResources)
			SHADER_PARAMETER_STRUCT_INCLUDE(IrradianceVolumeParameters, IrrVolumeParameters)
			SHADER_PARAMETER_STRUCT_INCLUDE(WorldRadianceCacheParameters, WRCParameters)
			SHADER_PARAMETER_STRUCT_INCLUDE(SpatialHashTableParameters, SpatialHashTableParams)
			SHADER_PARAMETER_STRUCT_INCLUDE(BlueNoiseShaderParameters, BlueNoise)
			SHADER_PARAMETER(uint32_t, IrradianceVolumeCascadeIndex)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(TraceIrradianceVolumeCascadeCS, "Engine/Shaders/Source/GlobalIllumination/TraceIrradianceVolumeCascadeCS.hlsl", "TraceIrradianceVolumeCascadeCS", Compute);

	struct WorldRadianceCacheShadeCellsCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(WorldRadianceCacheShadeCellsCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			RG_BUFFER_ACCESS(IndirectArgs, RGResourceAccess::IndirectArg)
			SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
			SHADER_PARAMETER_BUFFER_UAV(RWStructuredBuffer<uint>, RWWorldRadianceCacheCellCache)
			SHADER_PARAMETER_BUFFER_UAV(RWStructuredBuffer<uint>, RWWorldRadianceCacheCellInfo)
			SHADER_PARAMETER_BUFFER_SRV(ByteAddressBuffer, RayInfo)
			SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<uint>, WorldRadianceCacheCellsToShade)
			SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<uint2>, WorldRadianceCacheCellShadingInfo)

			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float3>, ProbeAtlas)

			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, DFGLuT)
			SHADER_PARAMETER_TEXTURE_SRV(TextureCube<float3>, SkylightIrradiance)
			SHADER_PARAMETER_TEXTURE_SRV(TextureCube<float3>, SkylightRadiance)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2DArray<float>, CascadedDirectionalShadowMap)
			SHADER_PARAMETER_UNIFORM_BUFFER(CascadedDirectionalLightShadowMappingData, CascadedDirectionalLightShadowMapping)
			SHADER_PARAMETER_SAMPLER(LinearSampler)
			SHADER_PARAMETER_SAMPLER(ShadowSampler)
			SHADER_PARAMETER(uint, NumRadianceMipLevels)

			SHADER_PARAMETER_RAY_TRACING_RESOURCE_TABLE(RTResources)
			SHADER_PARAMETER_STRUCT_INCLUDE(GPUSceneParameters, GPUScene)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(WorldRadianceCacheShadeCellsCS, "Engine/Shaders/Source/GlobalIllumination/WorldRadianceCacheShadeCellsCS.hlsl", "WorldRadianceCacheShadeCellsCS", Compute);

	struct PropagateRaysFromWorldRadianceCacheCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(PropagateRaysFromWorldRadianceCacheCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float3>, RWProbeAtlas)
			SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<uint>, WorldRadianceCacheCellCache)
			SHADER_PARAMETER_BUFFER_SRV(ByteAddressBuffer, RayInfo)
			SHADER_PARAMETER_STRUCT_INCLUDE(IrradianceVolumeParameters, IrrVolumeParameters)
			SHADER_PARAMETER_STRUCT_INCLUDE(WorldRadianceCacheParameters, WRCParameters)
			SHADER_PARAMETER_STRUCT_INCLUDE(SpatialHashTableParameters, SpatialHashTableParams)
			SHADER_PARAMETER(uint32_t, IrradianceVolumeCascadeIndex)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(PropagateRaysFromWorldRadianceCacheCS, "Engine/Shaders/Source/GlobalIllumination/PropagateRaysFromWorldRadianceCacheCS.hlsl", "PropagateRaysFromWorldRadianceCacheCS", Compute);

	struct VisualizeWorldRadianceCacheCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(VisualizeWorldRadianceCacheCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float4>, RWSceneColor)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float>, SceneDepth)
			SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<uint>, WorldRadianceCacheCellCache)
			SHADER_PARAMETER_STRUCT_INCLUDE(WorldRadianceCacheParameters, WRCParameters)
			SHADER_PARAMETER_STRUCT_INCLUDE(SpatialHashTableParameters, SpatialHashTableParams)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(VisualizeWorldRadianceCacheCS, "Engine/Shaders/Source/GlobalIllumination/VisualizeWorldRadianceCacheCS.hlsl", "VisualizeWorldRadianceCacheCS", Compute);

	struct VisualizeIrradianceVolumeVS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(VisualizeIrradianceVolumeVS)

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			RG_BUFFER_ACCESS(IndexBuffer, RGResourceAccess::IndexBuffer)
			RG_BUFFER_ACCESS(VertexBuffer, RGResourceAccess::VertexBuffer)
			SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
			SHADER_PARAMETER_STRUCT_INCLUDE(IrradianceVolumeParameters, IrrVolumeParameters)
			SHADER_PARAMETER(uint32_t, IrradianceVolumeCascadeIndex)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(VisualizeIrradianceVolumeVS, "Engine/Shaders/Source/GlobalIllumination/IrradianceVolumeVisualization.hlsl", "VisualizeIrradianceVolumeVS", Vertex);

	struct VisualizeIrradianceVolumePS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(VisualizeIrradianceVolumePS)

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float3>, ProbeAtlas)
			SHADER_PARAMETER_STRUCT_INCLUDE(IrradianceVolumeParameters, IrrVolumeParameters)
			SHADER_PARAMETER(uint32_t, IrradianceVolumeCascadeIndex)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(VisualizeIrradianceVolumePS, "Engine/Shaders/Source/GlobalIllumination/IrradianceVolumeVisualization.hlsl", "VisualizeIrradianceVolumePS", Pixel);

	BEGIN_SHADER_PARAMETER_STRUCT(VisualizeIrradianceVolumeParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(VisualizeIrradianceVolumeVS::Parameters, VS)
		SHADER_PARAMETER_STRUCT_INCLUDE(VisualizeIrradianceVolumePS::Parameters, PS)
	END_SHADER_PARAMETER_STRUCT()

	struct IrradianceVolumeConstants
	{
		inline static constexpr uint32_t NumMaxCascades = 10;

		float4 cascadeMinCornerAndSpacing[NumMaxCascades];
	};

	static uint32_t GetNumCascades()
	{
		return glm::min(IrradianceVolumeConstants::NumMaxCascades, static_cast<uint32_t>(s_giIrradianceVolumeNumCascades.GetValue()));
	}

	static uint32_t GetIrradianceVolumeProbeAtlasResolution()
	{
		return 2048;
	}

	static bool AnyVisualizationEnabled()
	{
		return s_giVisualizeIrradianceVolume.GetValue()
			|| s_giVisualizeWorldRadianceCache.GetValue();	
	}

	WorldRadianceCacheParameters GlobalIlluminationRenderer::GetWorldRadianceCacheParameters()
	{
		WorldRadianceCacheParameters worldRadianceCacheParameters;
		worldRadianceCacheParameters.WorldRadianceCacheLodDistance = s_giWorldRadianceCacheLodDistance.GetValue();
		worldRadianceCacheParameters.WorldRadianceCacheCellSize = s_giWorldRadianceCacheCellSize.GetValue();
		worldRadianceCacheParameters.WorldRadianceCacheCellLifetime = s_giWorldRadianceCacheCellLifetime.GetValue();

		return worldRadianceCacheParameters;
	}

	IrradianceVolumeParameters GlobalIlluminationRenderer::GetIrradianceVolumeParameters(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view)
	{
		IrradianceVolumeParameters irradianceVolumeParameters;
		irradianceVolumeParameters.IrradianceVolumeBaseSpacing = s_giIrradianceVolumeSpacing.GetValue();
		irradianceVolumeParameters.IrradianceVolumeResolution = s_giIrradianceVolumeResolution.GetValue();
		irradianceVolumeParameters.IrradianceVolumeNumCascades = s_giIrradianceVolumeNumCascades.GetValue();
		irradianceVolumeParameters.IrradianceVolumeProbeResolution = s_giIrradianceVolumeProbeResolution.GetValue();
		irradianceVolumeParameters.IrradianceVolumeConstantData = SetupIrradianceVolumeUniformBuffer(renderGraph, view);
		irradianceVolumeParameters.IrradianceVolumeProbeAtlasResolution = 2048;

		return irradianceVolumeParameters;
	}

	SpatialHashTableParameters GlobalIlluminationRenderer::GetSpatialHashTableParameters(RenderGraph& renderGraph)
	{
		SpatialHashTableParameters spatialHashTableParameters;
		spatialHashTableParameters.RWSpatialHashTableChecksum = renderGraph.CreateUAV(renderGraph.RegisterExternalBuffer(m_spatialHashTableChecksumBuffer), RHI::PixelFormat::R32_UINT);
		spatialHashTableParameters.SpatialHashTableSize = s_giSpatialHashTableSize.GetValue();

		return spatialHashTableParameters;
	}

	GlobalIlluminationRenderer::GlobalIlluminationRenderer()
	{
		{
			RHI::BufferDesc desc{};
			desc.count = s_giSpatialHashTableSize.GetValue();
			desc.elementSize = sizeof(uint32_t);
			desc.memoryUsage = RHI::MemoryUsage::GPU;
			desc.usage = RHI::BufferUsage::TexelBuffer;
			desc.debugName = "GI.SpatialHashTableChecksum";

			m_spatialHashTableChecksumBuffer = RHI::StorageBuffer::Create(desc);
		}

		{
			RHI::BufferDesc desc{};
			desc.count = s_giSpatialHashTableSize.GetValue();
			desc.elementSize = sizeof(uint32_t);
			desc.memoryUsage = RHI::MemoryUsage::GPU;
			desc.usage = RHI::BufferUsage::StorageBuffer;
			desc.debugName = "GI.WorldRadianceCacheCellCache";

			m_worldRadianceCacheCellCache = RHI::StorageBuffer::Create(desc);
		}

		{
			RHI::BufferDesc desc{};
			desc.count = s_giSpatialHashTableSize.GetValue();
			desc.elementSize = sizeof(uint32_t);
			desc.memoryUsage = RHI::MemoryUsage::GPU;
			desc.usage = RHI::BufferUsage::StorageBuffer;
			desc.debugName = "GI.WorldRadianceCacheCellInfo";

			m_worldRadianceCacheCellInfo = RHI::StorageBuffer::Create(desc);
		}

		{
			RHI::ImageDesc desc{};
			desc.format = RHI::PixelFormat::B10G11R11_UFLOAT_PACK32;
			desc.usage = RHI::ImageUsage::Storage;
			desc.width = GetIrradianceVolumeProbeAtlasResolution();
			desc.height = GetIrradianceVolumeProbeAtlasResolution();
			desc.debugName = "GI.ProbeAtlas";

			m_irradianceVolumeProbeAtlas = RHI::Image::Create(desc);
		}
	}

	void GlobalIlluminationRenderer::Visualize(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view)
	{
		if (!AnyVisualizationEnabled())
		{
			return;
		}

		WorldRadianceCacheParameters worldRadianceCacheParameters = GetWorldRadianceCacheParameters();
		SpatialHashTableParameters spatialHashTableParameters = GetSpatialHashTableParameters(renderGraph);
		IrradianceVolumeParameters irradianceVolumeParameters = GetIrradianceVolumeParameters(renderGraph, blackboard, view);

		SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();

		if (s_giVisualizeWorldRadianceCache.GetValue())
		{
			VisualizeWorldRadianceCacheCS::Parameters* passParameters = renderGraph.AllocParameters<VisualizeWorldRadianceCacheCS::Parameters>();
			passParameters->View = view.viewUniformBuffer;
			passParameters->RWSceneColor = renderGraph.CreateUAV(sceneTextures.sceneColor);
			passParameters->WorldRadianceCacheCellCache = renderGraph.CreateSRV(renderGraph.RegisterExternalBuffer(m_worldRadianceCacheCellCache));
			passParameters->SceneDepth = renderGraph.CreateSRV(sceneTextures.sceneDepth);
			passParameters->SpatialHashTableParams = spatialHashTableParameters;
			passParameters->WRCParameters = worldRadianceCacheParameters;

			auto shader = ShaderMap::Get<VisualizeWorldRadianceCacheCS>();
			ComputeShaderUtils::AddPass<VisualizeWorldRadianceCacheCS>(renderGraph,
				"VisualizeSpatialHashTable",
				shader,
				passParameters,
				{ Math::DivideRoundUp(view.width, 8u), Math::DivideRoundUp(view.height, 8u), 1u }
			);
		}

		if (s_giVisualizeIrradianceVolume.GetValue())
		{
			Ref<Mesh> sphereMesh = ShapeLibrary::GetSphere();

			RGBufferRef indexBuffer = renderGraph.RegisterExternalBuffer(sphereMesh->GetIndexBuffer());
			RGBufferRef vertexBuffer = renderGraph.RegisterExternalBuffer(sphereMesh->GetVertexPositionsBuffer());
			const uint32_t numIndices = static_cast<uint32_t>(sphereMesh->GetIndexCount());

			for (uint32_t i = 0; i < GetNumCascades(); ++i)
			{
				VisualizeIrradianceVolumeParameters* passParameters = renderGraph.AllocParameters<VisualizeIrradianceVolumeParameters>();
				passParameters->VS.IndexBuffer = indexBuffer;
				passParameters->VS.VertexBuffer = vertexBuffer;
				passParameters->VS.IrrVolumeParameters = irradianceVolumeParameters;
				passParameters->VS.IrradianceVolumeCascadeIndex = i;
				passParameters->VS.View = view.viewUniformBuffer;
				passParameters->PS.renderTargets.renderTargets[0] = sceneTextures.sceneColor;
				passParameters->PS.renderTargets.depthTarget = sceneTextures.sceneDepth;
				passParameters->PS.IrrVolumeParameters = irradianceVolumeParameters;
				passParameters->PS.ProbeAtlas = renderGraph.CreateSRV(renderGraph.RegisterExternalTexture(m_irradianceVolumeProbeAtlas));
				passParameters->PS.IrradianceVolumeCascadeIndex = i;

				auto vertexShader = ShaderMap::Get<VisualizeIrradianceVolumeVS>();
				auto pixelShader = ShaderMap::Get<VisualizeIrradianceVolumePS>();

				renderGraph.AddPass("VisualizeIrradianceVolume",
					RenderGraphPassFlags::None,
					passParameters,
					[passParameters, view, vertexShader, pixelShader, indexBuffer, vertexBuffer, numIndices](RenderContext& context)
				{
					RHI::RenderPipelineCreateInfo pipelineInfo{};
					pipelineInfo.shaders = { vertexShader, pixelShader };
					pipelineInfo.cullMode = RHI::CullMode::Back;
					pipelineInfo.depthMode = RHI::DepthMode::ReadWrite;

					auto pipeline = PipelineStateCache::GetRenderPipeline(pipelineInfo);

					RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->PS.renderTargets);
					renderingInfo.renderingInfo.colorAttachments[0].clearMode = RHI::ClearMode::Load;
					renderingInfo.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;

					const uint32_t numProbesPerClipmap = s_giIrradianceVolumeResolution.GetValue() * s_giIrradianceVolumeResolution.GetValue() * s_giIrradianceVolumeResolution.GetValue();

					context.BeginRendering(renderingInfo);
					context.BindPipeline(pipeline);
					context.BindIndexBuffer(indexBuffer);
					context.BindVertexBuffers({ vertexBuffer }, 0);
					context.SetParameters<VisualizeIrradianceVolumeVS>(vertexShader, &passParameters->VS);
					context.SetParameters<VisualizeIrradianceVolumePS>(pixelShader, &passParameters->PS);
					context.DrawIndexed(numIndices, numProbesPerClipmap, 0, 0, 0);
					context.EndRendering();
				});
			}
		}
	}

	GlobalIlluminationRenderer::Output GlobalIlluminationRenderer::Execute(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view)
	{
		if (!view.renderScene->GetRayTracingScene()->IsValid())
		{
			return {};
		}

		WorldRadianceCacheParameters worldRadianceCacheParameters = GetWorldRadianceCacheParameters();
		SpatialHashTableParameters spatialHashTableParameters = GetSpatialHashTableParameters(renderGraph);
		IrradianceVolumeParameters irradianceVolumeParameters = GetIrradianceVolumeParameters(renderGraph, blackboard, view);

		BlueNoiseShaderParameters blueNoiseParameters = BlueNoise::GetBlueNoiseParameters(renderGraph);

		SystemTextures& systemTextures = blackboard.Get<SystemTextures>();

		const uint32_t numProbesPerCascade = s_giIrradianceVolumeResolution.GetValue() * s_giIrradianceVolumeResolution.GetValue() * s_giIrradianceVolumeResolution.GetValue();
		const uint32_t numTotalRays = numProbesPerCascade * 64;

		const uint32_t cascadeToUpdateIndex = view.frameIndex % GetNumCascades();

		RGBufferRef worldRadianceCacheCellCacheBuffer = renderGraph.RegisterExternalBuffer(m_worldRadianceCacheCellCache);
		RGBufferRef worldRadianceCacheCellInfoBuffer = renderGraph.RegisterExternalBuffer(m_worldRadianceCacheCellInfo);

		RGBufferUAVRef worldRadianceCacheCellCacheUAV = renderGraph.CreateUAV(worldRadianceCacheCellCacheBuffer);
		RGBufferUAVRef worldRadianceCacheCellInfoUAV = renderGraph.CreateUAV(worldRadianceCacheCellInfoBuffer);

		{
			WorldRadianceCacheUpdateCS::Parameters* passParameters = renderGraph.AllocParameters<WorldRadianceCacheUpdateCS::Parameters>();
			passParameters->RWWorldRadianceCacheCellCache = worldRadianceCacheCellCacheUAV;
			passParameters->RWWorldRadianceCacheCellInfo = worldRadianceCacheCellInfoUAV;
			passParameters->SpatialHashTableParams = spatialHashTableParameters;

			auto shader = ShaderMap::Get<WorldRadianceCacheUpdateCS>();
			ComputeShaderUtils::AddPass<WorldRadianceCacheUpdateCS>(renderGraph,
				"WorldRadianceCacheUpdateCS",
				shader,
				passParameters,
				RenderGraphPassFlags::NeverCull,
				{ Math::DivideRoundUp(static_cast<uint32_t>(s_giSpatialHashTableSize.GetValue()), 64u), 1u, 1u }
			);
		}

		RGBufferRef rayInfoBuffer = renderGraph.CreateBuffer(RGBufferDesc::CreateByteAddressDesc(numTotalRays * sizeof(uint32_t) * 6, "GI.RayInfo"));
		RGBufferRef worldRadianceCacheCellMarkBuffer = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(Math::DivideRoundUp(s_giSpatialHashTableSize.GetValue(), 32), "GI.WorldRadianceCache.Mark"));
		RGBufferRef worldRadianceCacheCellsToShadeBuffer = renderGraph.CreateBuffer(RGBufferDesc::CreateStructuredBufferDesc<uint32_t>(numTotalRays, "GI.WorldRadianceCache.CellsToShade"));
		RGBufferRef worldRadianceCacheCellShadingInfoBuffer = renderGraph.CreateBuffer(RGBufferDesc::CreateStructuredBufferDesc<glm::uvec2>(s_giSpatialHashTableSize.GetValue(), "GI.WorldRadianceCache.ShadingInfo"));
		RGBufferRef cellShadingIndirectArgsBuffer = renderGraph.CreateBuffer(RGBufferDesc::CreateIndirectDesc<RHI::DispatchIndirectCommand>(1, "GI.WorldRadianceCache.ShadingIndirectArgs"));

		{
			RGBufferUAVRef cellShadingIndirectArgsUAV = renderGraph.CreateUAV(cellShadingIndirectArgsBuffer, RHI::PixelFormat::R32_UINT);
			RGBufferUAVRef worldRadianceCacheCellMarkUAV = renderGraph.CreateUAV(worldRadianceCacheCellMarkBuffer, RHI::PixelFormat::R32_UINT);

			AddClearUAVPass(renderGraph, cellShadingIndirectArgsUAV, 0u);
			AddClearUAVPass(renderGraph, renderGraph.CreateUAV(RGBufferUAVDesc::Create(worldRadianceCacheCellsToShadeBuffer, sizeof(uint32_t))), 0u);
			AddClearUAVPass(renderGraph, worldRadianceCacheCellMarkUAV, 0u);

			TraceIrradianceVolumeCascadeCS::Parameters* passParameters = renderGraph.AllocParameters<TraceIrradianceVolumeCascadeCS::Parameters>();
			passParameters->View = view.viewUniformBuffer;
			passParameters->TLAS = view.renderScene->GetRayTracingScene()->GetAccelerationStructure();
			passParameters->RTResources = view.renderScene->GetRayTracingResourceTable();
			passParameters->RWRayInfo = renderGraph.CreateUAV(rayInfoBuffer);
			passParameters->RWWorldRadianceCacheCellsToShade = renderGraph.CreateUAV(worldRadianceCacheCellsToShadeBuffer);
			passParameters->RWWorldRadianceCacheCellShadingInfo = renderGraph.CreateUAV(worldRadianceCacheCellShadingInfoBuffer);
			passParameters->RWShadingIndirectArgs = cellShadingIndirectArgsUAV;
			passParameters->RWWorldRadianceCacheCellMark = worldRadianceCacheCellMarkUAV;
			passParameters->IrrVolumeParameters = irradianceVolumeParameters;
			passParameters->SpatialHashTableParams = spatialHashTableParameters;
			passParameters->WRCParameters = worldRadianceCacheParameters;
			passParameters->IrradianceVolumeCascadeIndex = cascadeToUpdateIndex;
			passParameters->BlueNoise = blueNoiseParameters;

			auto shader = ShaderMap::Get<TraceIrradianceVolumeCascadeCS>();
			ComputeShaderUtils::AddPass<TraceIrradianceVolumeCascadeCS>(renderGraph,
				"TraceIrradianceVolumeCascadeCS",
				shader,
				passParameters,
				RenderGraphPassFlags::NeverCull,
				{ Math::DivideRoundUp(numTotalRays, 64u), 1u, 1u }
			);
		}

		RGBufferSRVRef worldRadianceCacheCellCacheSRV = renderGraph.CreateSRV(worldRadianceCacheCellCacheBuffer);
		RGTextureRef probeAtlas = renderGraph.RegisterExternalTexture(m_irradianceVolumeProbeAtlas);

		RGBufferSRVRef rayInfoSRV = renderGraph.CreateSRV(rayInfoBuffer);

		{
			const EnvironmentTextures& environmentTextures = blackboard.Get<EnvironmentTextures>();
			const CascadedShadowMapsTechnique::Result& directionalShadowMap = blackboard.Get<CascadedShadowMapsTechnique::Result>();

			WorldRadianceCacheShadeCellsCS::Parameters* passParameters = renderGraph.AllocParameters<WorldRadianceCacheShadeCellsCS::Parameters>();
			passParameters->IndirectArgs = cellShadingIndirectArgsBuffer;
			passParameters->View = view.viewUniformBuffer;
			passParameters->RTResources = view.renderScene->GetRayTracingResourceTable();
			passParameters->GPUScene = view.renderScene->GetGPUSceneParameters(renderGraph);
			passParameters->RayInfo = rayInfoSRV;
			passParameters->WorldRadianceCacheCellsToShade = renderGraph.CreateSRV(worldRadianceCacheCellsToShadeBuffer);
			passParameters->WorldRadianceCacheCellShadingInfo = renderGraph.CreateSRV(worldRadianceCacheCellShadingInfoBuffer);
			passParameters->RWWorldRadianceCacheCellCache = worldRadianceCacheCellCacheUAV;
			passParameters->RWWorldRadianceCacheCellInfo = worldRadianceCacheCellInfoUAV;

			passParameters->ProbeAtlas = renderGraph.CreateSRV(probeAtlas);

			passParameters->DFGLuT = renderGraph.CreateSRV(environmentTextures.DFGLuT);
			passParameters->SkylightIrradiance = renderGraph.CreateSRV(environmentTextures.irradiance);
			passParameters->SkylightRadiance = renderGraph.CreateSRV(environmentTextures.radiance);
			passParameters->LinearSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Clamp>();
			passParameters->NumRadianceMipLevels = environmentTextures.radiance->GetDesc().mips;

			RGTextureRef directionalShadowTexture = directionalShadowMap.shadowMap;

			if (!directionalShadowTexture)
			{
				directionalShadowTexture = systemTextures.blackCubeTexture;
			}

			passParameters->CascadedDirectionalShadowMap = renderGraph.CreateSRV(directionalShadowTexture);
			passParameters->ShadowSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Repeat, RHI::AnisotropyLevel::None, RHI::CompareOperator::LessEqual>();
			passParameters->CascadedDirectionalLightShadowMapping = directionalShadowMap.uniformBuffer;

			auto shader = ShaderMap::Get<WorldRadianceCacheShadeCellsCS>();
			ComputeShaderUtils::AddPass<WorldRadianceCacheShadeCellsCS>(renderGraph,
				"WorldRadianceCacheShadeCellsCS",
				shader,
				passParameters,
				RenderGraphPassFlags::NeverCull,
				cellShadingIndirectArgsBuffer,
				0u
			);
		}

		{
			RGTextureUAVRef probeAtlasUAV = renderGraph.CreateUAV(probeAtlas);

			PropagateRaysFromWorldRadianceCacheCS::Parameters* passParameters = renderGraph.AllocParameters<PropagateRaysFromWorldRadianceCacheCS::Parameters>();
			passParameters->View = view.viewUniformBuffer;
			passParameters->RayInfo = rayInfoSRV;
			passParameters->WorldRadianceCacheCellCache = worldRadianceCacheCellCacheSRV;
			passParameters->RWProbeAtlas = probeAtlasUAV;
			passParameters->IrrVolumeParameters = irradianceVolumeParameters;
			passParameters->SpatialHashTableParams = spatialHashTableParameters;
			passParameters->WRCParameters = worldRadianceCacheParameters;
			passParameters->IrradianceVolumeCascadeIndex = cascadeToUpdateIndex;

			auto shader = ShaderMap::Get<PropagateRaysFromWorldRadianceCacheCS>();
			ComputeShaderUtils::AddPass<PropagateRaysFromWorldRadianceCacheCS>(renderGraph,
				"PropagateRaysFromWorldRadianceCacheCS",
				shader,
				passParameters,
				RenderGraphPassFlags::NeverCull,
				{ Math::DivideRoundUp(numTotalRays, 64u), 1u, 1u }
			);
		}

		RGTextureRef indirectLight = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "GI.IndirectLight"));
		RGTextureUAVRef indirectLightUAV = renderGraph.CreateUAV(indirectLight);

		// Final Gather
		{
			SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();

			FinalGatherCS::Parameters* passParameters = renderGraph.AllocParameters<FinalGatherCS::Parameters>();
			passParameters->View = view.viewUniformBuffer;
			passParameters->RWIndirectLight = renderGraph.CreateUAV(indirectLight);
			passParameters->GBufferAlbedo = renderGraph.CreateSRV(sceneTextures.gBufferAlbedo);
			passParameters->GBufferNormal = renderGraph.CreateSRV(sceneTextures.gBufferNormals);
			passParameters->GBufferMaterial = renderGraph.CreateSRV(sceneTextures.gBufferMaterial);
			passParameters->SceneDepth = renderGraph.CreateSRV(sceneTextures.sceneDepth);
			passParameters->WorldRadianceCacheCellCache = worldRadianceCacheCellCacheSRV;
			passParameters->TLAS = view.renderScene->GetRayTracingScene()->GetAccelerationStructure();
			passParameters->RTResources = view.renderScene->GetRayTracingResourceTable();
			passParameters->GPUScene = view.renderScene->GetGPUSceneParameters(renderGraph);
			passParameters->BlueNoise = BlueNoise::GetBlueNoiseParameters(renderGraph);
			passParameters->SpatialHashTableParams = spatialHashTableParameters;
			passParameters->WRCParameters = worldRadianceCacheParameters;

			auto shader = ShaderMap::Get<FinalGatherCS>();
			ComputeShaderUtils::AddPass<FinalGatherCS>(renderGraph,
				"GI.FinalGather",
				shader,
				passParameters,
				RenderGraphPassFlags::NeverCull,
				{ Math::DivideRoundUp(view.width, 8u), Math::DivideRoundUp(view.height, 8u), 1u });
		}

		// Temporal Accumulation
		{
			RGTextureRef prevIndirectLight = nullptr;
			RGTextureUAVRef prevIndirectLightUAV = nullptr;

			if (m_prevIndirectLight == nullptr || (m_prevIndirectLight->GetWidth() != view.width || m_prevIndirectLight->GetHeight() != view.height))
			{
				prevIndirectLight = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(view.width, view.height, RHI::ImageUsage::Storage, "GI.Accumulation"));
				renderGraph.EnqueueTextureExtraction(prevIndirectLight, &m_prevIndirectLight);
				prevIndirectLightUAV = renderGraph.CreateUAV(prevIndirectLight);
			}
			else
			{
				prevIndirectLight = renderGraph.RegisterExternalTexture(m_prevIndirectLight);
				prevIndirectLightUAV = renderGraph.CreateUAV(prevIndirectLight);
			}

			TemporalAccumulationCS::Parameters* passParameters = renderGraph.AllocParameters<TemporalAccumulationCS::Parameters>();
			passParameters->RWIndirectLight = indirectLightUAV;
			passParameters->RWPrevFrameIndirectLight = prevIndirectLightUAV;
			passParameters->AccumulationAlpha = 0.1f;

			auto shader = ShaderMap::Get<TemporalAccumulationCS>();
			ComputeShaderUtils::AddPass<TemporalAccumulationCS>(renderGraph,
				"GI.TemporalAccumulation",
				shader,
				passParameters,
				RenderGraphPassFlags::NeverCull,
				{ Math::DivideRoundUp(view.width, 8u), Math::DivideRoundUp(view.height, 8u), 1u });
		}

		Output result;
		result.indirectLight = indirectLight;

		return result;
	}

	RGUniformBufferRef GlobalIlluminationRenderer::SetupIrradianceVolumeUniformBuffer(RenderGraph& renderGraph, const RenderView& view)
	{
		RGUniformBufferRef uniformBuffer = renderGraph.CreateUniformBuffer(RGUniformBufferDesc::Create<IrradianceVolumeConstants>("GI.IrradianceVolumeConstants"));

		IrradianceVolumeConstants constants{};

		glm::vec3 cameraPosition = 0.f;

		if (!s_giIrradianceVolumeFreezeAtWorldOrigin.GetValue())
		{
			cameraPosition = view.camera->GetPosition();
		}

		for (uint32_t i = 0; i < GetNumCascades(); ++i)
		{
			const float cascadeSpacing = s_giIrradianceVolumeSpacing.GetValue() * std::powf(2.f, static_cast<float>(i));

			glm::vec3 origin = cameraPosition;
			origin /= cascadeSpacing;
			origin = glm::floor(origin);
			origin *= cascadeSpacing;

			const glm::vec3 gridMin = origin - cascadeSpacing * (s_giIrradianceVolumeResolution.GetValue() * 0.5f);
			constants.cascadeMinCornerAndSpacing[i] = glm::vec4(gridMin, cascadeSpacing);
		}

		AddMappedBufferUpload(renderGraph, uniformBuffer, &constants, sizeof(IrradianceVolumeConstants));

		return uniformBuffer;
	}
}
