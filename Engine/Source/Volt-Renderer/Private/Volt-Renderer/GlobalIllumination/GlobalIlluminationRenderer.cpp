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

#include <CoreModule/Console/ConsoleVariableRegistry.h>

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
		0,
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
		0,
		"Whether or not to freeze the irradiance volume at world origin."
	);

	static ConsoleVariable<int32_t> s_giIrradianceVolumeFreeze(
		"r.GI.IrradianceVolume.Freeze",
		0,
		"Whether or not to freeze the irradiance volume at the current position."
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
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float3>, ProbeAtlas)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float2>, ProbeVisibilityAtlas)
			SHADER_PARAMETER_BUFFER_SRV(Buffer<uint>, ProbeStatus)
			SHADER_PARAMETER_SAMPLER(BilinearSampler)
			SHADER_PARAMETER_ACCELERATION_STRUCTURE(TLAS)
			SHADER_PARAMETER_RESOURCE_TABLE(RTResources)
			SHADER_PARAMETER_STRUCT_INCLUDE(GPUSceneParameters, GPUScene)
			SHADER_PARAMETER_STRUCT_INCLUDE(BlueNoiseShaderParameters, BlueNoise)
			SHADER_PARAMETER_STRUCT_INCLUDE(SpatialHashTableParameters, SpatialHashTableParams)
			SHADER_PARAMETER_STRUCT_INCLUDE(WorldRadianceCacheParameters, WRCParameters)
			SHADER_PARAMETER_STRUCT_INCLUDE(IrradianceVolumeParameters, IrrVolumeParameters)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(FinalGatherCS, "Engine/Shaders/Source/GlobalIllumination/FinalGather.hlsl", "FinalGatherCS", Compute);

	struct TemporalAccumulationCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(TemporalAccumulationCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float4>, RWIndirectLight)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float4>, RWPrevFrameIndirectLight)
			SHADER_PARAMETER(float, AccumulationAlpha)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(TemporalAccumulationCS, "Engine/Shaders/Source/GlobalIllumination/TemporalAccumulation.hlsl", "TemporalAccumulationCS", Compute);
	
	struct WorldRadianceCacheUpdateCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(WorldRadianceCacheUpdateCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER_UAV(RWStructuredBuffer<uint>, RWWorldRadianceCacheCellCache)
			SHADER_PARAMETER_BUFFER_UAV(RWStructuredBuffer<uint>, RWWorldRadianceCacheCellInfo)
			SHADER_PARAMETER_STRUCT_INCLUDE(SpatialHashTableParameters, SpatialHashTableParams)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(WorldRadianceCacheUpdateCS, "Engine/Shaders/Source/GlobalIllumination/WorldRadianceCacheUpdateCS.hlsl", "WorldRadianceCacheUpdateCS", Compute);

	struct IrradianceVolumeTraceCascadeCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(IrradianceVolumeTraceCascadeCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
			SHADER_PARAMETER_BUFFER_UAV(RWByteAddressBuffer, RWRayInfo)
			SHADER_PARAMETER_BUFFER_UAV(RWStructuredBuffer<uint>, RWWorldRadianceCacheCellsToShade)
			SHADER_PARAMETER_BUFFER_UAV(RWStructuredBuffer<uint2>, RWWorldRadianceCacheCellShadingInfo)
			SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWShadingIndirectArgs)
			SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWWorldRadianceCacheCellMark)
			SHADER_PARAMETER_ACCELERATION_STRUCTURE(TLAS)
			SHADER_PARAMETER_RESOURCE_TABLE(RTResources)
			SHADER_PARAMETER_STRUCT_INCLUDE(IrradianceVolumeParameters, IrrVolumeParameters)
			SHADER_PARAMETER_STRUCT_INCLUDE(WorldRadianceCacheParameters, WRCParameters)
			SHADER_PARAMETER_STRUCT_INCLUDE(SpatialHashTableParameters, SpatialHashTableParams)
			SHADER_PARAMETER_STRUCT_INCLUDE(BlueNoiseShaderParameters, BlueNoise)
			SHADER_PARAMETER(uint32_t, IrradianceVolumeCascadeIndex)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(IrradianceVolumeTraceCascadeCS, "Engine/Shaders/Source/GlobalIllumination/IrradianceVolumeTraceCascadeCS.hlsl", "TraceIrradianceVolumeCascadeCS", Compute);

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
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float2>, ProbeVisibilityAtlas)
			SHADER_PARAMETER_BUFFER_SRV(Buffer<uint>, ProbeStatus)
			SHADER_PARAMETER_STRUCT_INCLUDE(IrradianceVolumeParameters, IrrVolumeParameters)
			SHADER_PARAMETER_SAMPLER(BilinearSampler)

			SHADER_PARAMETER_RESOURCE_TABLE(RTResources)
			SHADER_PARAMETER_ACCELERATION_STRUCTURE(TLAS)
			SHADER_PARAMETER_STRUCT_INCLUDE(GPUSceneParameters, GPUScene)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(WorldRadianceCacheShadeCellsCS, "Engine/Shaders/Source/GlobalIllumination/WorldRadianceCacheShadeCellsCS.hlsl", "WorldRadianceCacheShadeCellsCS", Compute);

	struct PropagateRaysFromWorldRadianceCacheCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(PropagateRaysFromWorldRadianceCacheCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float3>, RWProbeAtlas)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float2>, RWProbeVisibilityAtlas)
			SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWProbeStatus)
			SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<uint>, WorldRadianceCacheCellCache)
			SHADER_PARAMETER_BUFFER_SRV(ByteAddressBuffer, RayInfo)
			SHADER_PARAMETER_STRUCT_INCLUDE(IrradianceVolumeParameters, IrrVolumeParameters)
			SHADER_PARAMETER_STRUCT_INCLUDE(WorldRadianceCacheParameters, WRCParameters)
			SHADER_PARAMETER_STRUCT_INCLUDE(SpatialHashTableParameters, SpatialHashTableParams)
			SHADER_PARAMETER(uint32_t, IrradianceVolumeCascadeIndex)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(PropagateRaysFromWorldRadianceCacheCS, "Engine/Shaders/Source/GlobalIllumination/PropagateRaysFromWorldRadianceCacheCS.hlsl", "PropagateRaysFromWorldRadianceCacheCS", Compute);

	struct IrradianceVolumeFillProbeBordersCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(IrradianceVolumeFillProbeBordersCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float3>, RWProbeAtlas)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float2>, RWProbeVisibilityAtlas)
			SHADER_PARAMETER_STRUCT_INCLUDE(IrradianceVolumeParameters, IrrVolumeParameters)
			SHADER_PARAMETER(uint32_t, IrradianceVolumeCascadeIndex)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(IrradianceVolumeFillProbeBordersCS, "Engine/Shaders/Source/GlobalIllumination/IrradianceVolumeFillProbeBordersCS.hlsl", "IrradianceVolumeFillProbeBordersCS", Compute);

	struct WorldRadianceCacheVisualizeCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(WorldRadianceCacheVisualizeCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float4>, RWSceneColor)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float>, SceneDepth)
			SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<uint>, WorldRadianceCacheCellCache)
			SHADER_PARAMETER_STRUCT_INCLUDE(WorldRadianceCacheParameters, WRCParameters)
			SHADER_PARAMETER_STRUCT_INCLUDE(SpatialHashTableParameters, SpatialHashTableParams)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(WorldRadianceCacheVisualizeCS, "Engine/Shaders/Source/GlobalIllumination/WorldRadianceCacheVisualizeCS.hlsl", "VisualizeWorldRadianceCacheCS", Compute);

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
	VT_REGISTER_SHADER(VisualizeIrradianceVolumeVS, "Engine/Shaders/Source/GlobalIllumination/IrradianceVolumeVisualization.hlsl", "VisualizeIrradianceVolumeVS", Vertex);

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
	VT_REGISTER_SHADER(VisualizeIrradianceVolumePS, "Engine/Shaders/Source/GlobalIllumination/IrradianceVolumeVisualization.hlsl", "VisualizeIrradianceVolumePS", Pixel);

	BEGIN_SHADER_PARAMETER_STRUCT(VisualizeIrradianceVolumeParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(VisualizeIrradianceVolumeVS::Parameters, VS)
		SHADER_PARAMETER_STRUCT_INCLUDE(VisualizeIrradianceVolumePS::Parameters, PS)
	END_SHADER_PARAMETER_STRUCT()

	static uint32_t GetNumCascades()
	{
		return glm::min(GlobalIlluminationRenderer::IrradianceVolumeConstants::NumMaxCascades, static_cast<uint32_t>(s_giIrradianceVolumeNumCascades.GetValue()));
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
			desc.numElements = s_giSpatialHashTableSize.GetValue();
			desc.elementSize = sizeof(uint32_t);
			desc.memoryUsage = RHI::MemoryUsage::GPU;
			desc.usage = RHI::BufferUsage::TexelBuffer;
			desc.debugName = "GI.SpatialHashTableChecksum";

			m_spatialHashTableChecksumBuffer = RHI::Buffer::Create(desc);
		}

		{
			RHI::BufferDesc desc{};
			desc.numElements = s_giSpatialHashTableSize.GetValue();
			desc.elementSize = sizeof(uint32_t);
			desc.memoryUsage = RHI::MemoryUsage::GPU;
			desc.usage = RHI::BufferUsage::StorageBuffer;
			desc.debugName = "GI.WorldRadianceCacheCellCache";

			m_worldRadianceCacheCellCache = RHI::Buffer::Create(desc);
		}

		{
			RHI::BufferDesc desc{};
			desc.numElements = s_giSpatialHashTableSize.GetValue();
			desc.elementSize = sizeof(uint32_t);
			desc.memoryUsage = RHI::MemoryUsage::GPU;
			desc.usage = RHI::BufferUsage::StorageBuffer;
			desc.debugName = "GI.WorldRadianceCacheCellInfo";

			m_worldRadianceCacheCellInfo = RHI::Buffer::Create(desc);
		}

		{
			RHI::ImageDesc desc{};
			desc.format = RHI::PixelFormat::B10G11R11_UFLOAT_PACK32;
			desc.usage = RHI::ImageUsage::Storage;
			desc.width = GetIrradianceVolumeProbeAtlasResolution();
			desc.height = GetIrradianceVolumeProbeAtlasResolution();
			desc.debugName = "GI.ProbeRadianceAtlas";

			m_irradianceVolumeProbeRadianceAtlas = RHI::Image::Create(desc);
		}

		{
			RHI::ImageDesc desc{};
			desc.format = RHI::PixelFormat::R32G32_SFLOAT;
			desc.usage = RHI::ImageUsage::Storage;
			desc.width = GetIrradianceVolumeProbeAtlasResolution();
			desc.height = GetIrradianceVolumeProbeAtlasResolution();
			desc.debugName = "GI.ProbeVisibilityAtlas";

			m_irradianceVolumeProbeVisibilityAtlas = RHI::Image::Create(desc);
		}

		{
			RHI::BufferDesc desc{};
			desc.numElements = s_giIrradianceVolumeResolution.GetValue() * s_giIrradianceVolumeResolution.GetValue() * s_giIrradianceVolumeResolution.GetValue() * s_giIrradianceVolumeNumCascades.GetValue() * 3;
			desc.elementSize = sizeof(float);
			desc.memoryUsage = RHI::MemoryUsage::GPU;
			desc.usage = RHI::BufferUsage::TexelBuffer;
			desc.debugName = "GI.IrradianceVolumeProbeOffsets";
			
			m_irradianceVolumeProbeOffsets = RHI::Buffer::Create(desc);
		}

		{
			const uint32_t numTotalProbes = s_giIrradianceVolumeResolution.GetValue() * s_giIrradianceVolumeResolution.GetValue() * s_giIrradianceVolumeResolution.GetValue() * s_giIrradianceVolumeNumCascades.GetValue();

			RHI::BufferDesc desc{};
			desc.numElements = Math::DivideRoundUp(numTotalProbes, 32u);
			desc.elementSize = sizeof(uint32_t);
			desc.memoryUsage = RHI::MemoryUsage::GPU;
			desc.usage = RHI::BufferUsage::TexelBuffer;
			desc.debugName = "GI.IrradianceVolumeProbeStatus";

			m_irradianceVolumeProbeStatus = RHI::Buffer::Create(desc);
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
			WorldRadianceCacheVisualizeCS::Parameters* passParameters = renderGraph.AllocParameters<WorldRadianceCacheVisualizeCS::Parameters>();
			passParameters->View = view.viewUniformBuffer;
			passParameters->RWSceneColor = renderGraph.CreateUAV(sceneTextures.sceneColor);
			passParameters->WorldRadianceCacheCellCache = renderGraph.CreateSRV(renderGraph.RegisterExternalBuffer(m_worldRadianceCacheCellCache));
			passParameters->SceneDepth = renderGraph.CreateSRV(sceneTextures.sceneDepth);
			passParameters->SpatialHashTableParams = spatialHashTableParameters;
			passParameters->WRCParameters = worldRadianceCacheParameters;

			auto shader = ShaderMap::Get<WorldRadianceCacheVisualizeCS>();
			ComputeShaderUtils::AddPass<WorldRadianceCacheVisualizeCS>(renderGraph,
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
				passParameters->PS.ProbeAtlas = renderGraph.CreateSRV(renderGraph.RegisterExternalTexture(m_irradianceVolumeProbeRadianceAtlas));
				passParameters->PS.IrradianceVolumeCascadeIndex = i;

				auto vertexShader = ShaderMap::Get<VisualizeIrradianceVolumeVS>();
				auto pixelShader = ShaderMap::Get<VisualizeIrradianceVolumePS>();

				renderGraph.AddPass("VisualizeIrradianceVolume",
					RenderGraphPassFlags::None,
					passParameters,
					[passParameters, view, vertexShader, pixelShader, indexBuffer, vertexBuffer, numIndices](RenderContext& context)
				{
					GraphicsPipelineState pipelineState{};
					pipelineState.shaders = { vertexShader, pixelShader };
					pipelineState.cullMode = RHI::CullMode::Back;
					pipelineState.depthMode = RHI::DepthMode::ReadWrite;
					pipelineState.renderTargets = passParameters->PS.renderTargets;

					RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->PS.renderTargets);
					renderingInfo.renderingInfo.colorAttachments[0].clearMode = RHI::ClearMode::Load;
					renderingInfo.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;

					const uint32_t numProbesPerClipmap = s_giIrradianceVolumeResolution.GetValue() * s_giIrradianceVolumeResolution.GetValue() * s_giIrradianceVolumeResolution.GetValue();

					context.BeginRendering(renderingInfo);
					context.SetPipelineState(pipelineState);
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

		RGTextureRef probeRadianceAtlas = renderGraph.RegisterExternalTexture(m_irradianceVolumeProbeRadianceAtlas);
		RGTextureRef probeVisibilityAtlas = renderGraph.RegisterExternalTexture(m_irradianceVolumeProbeVisibilityAtlas);
		RGBufferRef probeOffsets = renderGraph.RegisterExternalBuffer(m_irradianceVolumeProbeOffsets);
		RGBufferRef probeStatus = renderGraph.RegisterExternalBuffer(m_irradianceVolumeProbeStatus);

		static bool shouldInvalidate = true;
		if (shouldInvalidate)
		{
			AddClearUAVPass(renderGraph, renderGraph.CreateUAV(probeRadianceAtlas), glm::vec4{ 0.f });
			AddClearUAVPass(renderGraph, renderGraph.CreateUAV(probeVisibilityAtlas), glm::vec4{ 0.f });
			AddClearUAVPass(renderGraph, renderGraph.CreateUAV(probeOffsets, RHI::PixelFormat::R32_UINT), 0.f);
			AddClearUAVPass(renderGraph, renderGraph.CreateUAV(probeStatus, RHI::PixelFormat::R32_UINT), 0.f);
			shouldInvalidate = false;
		}

		WorldRadianceCacheParameters worldRadianceCacheParameters = GetWorldRadianceCacheParameters();
		SpatialHashTableParameters spatialHashTableParameters = GetSpatialHashTableParameters(renderGraph);
		IrradianceVolumeParameters irradianceVolumeParameters = GetIrradianceVolumeParameters(renderGraph, blackboard, view);

		BlueNoiseShaderParameters blueNoiseParameters = BlueNoise::GetBlueNoiseParameters(renderGraph);

		const uint32_t numProbesPerCascade = s_giIrradianceVolumeResolution.GetValue() * s_giIrradianceVolumeResolution.GetValue() * s_giIrradianceVolumeResolution.GetValue();
		const uint32_t numTotalRays = numProbesPerCascade * 64;

		const uint32_t cascadeToUpdateIndex = view.frameIndex % GetNumCascades();

		RGBufferRef worldRadianceCacheCellCacheBuffer = renderGraph.RegisterExternalBuffer(m_worldRadianceCacheCellCache);
		RGBufferRef worldRadianceCacheCellInfoBuffer = renderGraph.RegisterExternalBuffer(m_worldRadianceCacheCellInfo);

		RGBufferUAVRef worldRadianceCacheCellCacheUAV = renderGraph.CreateUAV(worldRadianceCacheCellCacheBuffer);
		RGBufferUAVRef worldRadianceCacheCellInfoUAV = renderGraph.CreateUAV(worldRadianceCacheCellInfoBuffer);

		if (s_giWorldRadianceCacheCellLifetime.GetValue() != 0)
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

			IrradianceVolumeTraceCascadeCS::Parameters* passParameters = renderGraph.AllocParameters<IrradianceVolumeTraceCascadeCS::Parameters>();
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

			auto shader = ShaderMap::Get<IrradianceVolumeTraceCascadeCS>();
			ComputeShaderUtils::AddPass<IrradianceVolumeTraceCascadeCS>(renderGraph,
				"TraceIrradianceVolumeCascadeCS",
				shader,
				passParameters,
				{ Math::DivideRoundUp(numTotalRays, 64u), 1u, 1u }
			);
		}

		RGBufferSRVRef worldRadianceCacheCellCacheSRV = renderGraph.CreateSRV(worldRadianceCacheCellCacheBuffer);

		RGBufferSRVRef rayInfoSRV = renderGraph.CreateSRV(rayInfoBuffer);

		{
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

			passParameters->ProbeAtlas = renderGraph.CreateSRV(probeRadianceAtlas);
			passParameters->ProbeVisibilityAtlas = renderGraph.CreateSRV(probeVisibilityAtlas);
			passParameters->ProbeStatus = renderGraph.CreateSRV(probeStatus, RHI::PixelFormat::R32_UINT);
			passParameters->IrrVolumeParameters = irradianceVolumeParameters;
			passParameters->BilinearSampler = SamplerStateCache::GetBilinearSampler();

			passParameters->TLAS = view.renderScene->GetRayTracingScene()->GetAccelerationStructure();

			auto shader = ShaderMap::Get<WorldRadianceCacheShadeCellsCS>();
			ComputeShaderUtils::AddPass<WorldRadianceCacheShadeCellsCS>(renderGraph,
				"WorldRadianceCacheShadeCellsCS",
				shader,
				passParameters,
				cellShadingIndirectArgsBuffer,
				0u
			);
		}

		RGTextureUAVRef probeRadianceAtlasUAV = renderGraph.CreateUAV(probeRadianceAtlas);
		RGTextureUAVRef probeVisibilityAtlasUAV = renderGraph.CreateUAV(probeVisibilityAtlas);
		RGBufferUAVRef probeStatusUAV = renderGraph.CreateUAV(probeStatus, RHI::PixelFormat::R32_UINT);
		{
			PropagateRaysFromWorldRadianceCacheCS::Parameters* passParameters = renderGraph.AllocParameters<PropagateRaysFromWorldRadianceCacheCS::Parameters>();
			passParameters->View = view.viewUniformBuffer;
			passParameters->RayInfo = rayInfoSRV;
			passParameters->WorldRadianceCacheCellCache = worldRadianceCacheCellCacheSRV;
			passParameters->RWProbeAtlas = probeRadianceAtlasUAV;
			passParameters->RWProbeVisibilityAtlas = probeVisibilityAtlasUAV;
			passParameters->RWProbeStatus = probeStatusUAV;
			passParameters->IrrVolumeParameters = irradianceVolumeParameters;
			passParameters->SpatialHashTableParams = spatialHashTableParameters;
			passParameters->WRCParameters = worldRadianceCacheParameters;
			passParameters->IrradianceVolumeCascadeIndex = cascadeToUpdateIndex;

			auto shader = ShaderMap::Get<PropagateRaysFromWorldRadianceCacheCS>();
			ComputeShaderUtils::AddPass<PropagateRaysFromWorldRadianceCacheCS>(renderGraph,
				"PropagateRaysFromWorldRadianceCacheCS",
				shader,
				passParameters,
				{ Math::DivideRoundUp(numTotalRays, 64u), 1u, 1u }
			);
		}

		{
			IrradianceVolumeFillProbeBordersCS::Parameters* passParameters = renderGraph.AllocParameters<IrradianceVolumeFillProbeBordersCS::Parameters>();
			passParameters->RWProbeAtlas = probeRadianceAtlasUAV;
			passParameters->RWProbeVisibilityAtlas = probeVisibilityAtlasUAV;
			passParameters->IrrVolumeParameters = irradianceVolumeParameters;
			passParameters->IrradianceVolumeCascadeIndex = cascadeToUpdateIndex;
		
			auto shader = ShaderMap::Get<IrradianceVolumeFillProbeBordersCS>();
			ComputeShaderUtils::AddPass<IrradianceVolumeFillProbeBordersCS>(renderGraph,
				"IrradianceVolumeFillProbeBordersCS",
				shader,
				passParameters,
				{ numProbesPerCascade, 1u, 1u }
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
			passParameters->ProbeAtlas = renderGraph.CreateSRV(probeRadianceAtlas);
			passParameters->ProbeVisibilityAtlas = renderGraph.CreateSRV(probeVisibilityAtlas);
			passParameters->ProbeStatus = renderGraph.CreateSRV(probeStatus, RHI::PixelFormat::R32_UINT);
			passParameters->TLAS = view.renderScene->GetRayTracingScene()->GetAccelerationStructure();
			passParameters->RTResources = view.renderScene->GetRayTracingResourceTable();
			passParameters->BilinearSampler = SamplerStateCache::GetBilinearSampler();
			passParameters->GPUScene = view.renderScene->GetGPUSceneParameters(renderGraph);
			passParameters->BlueNoise = BlueNoise::GetBlueNoiseParameters(renderGraph);
			passParameters->SpatialHashTableParams = spatialHashTableParameters;
			passParameters->WRCParameters = worldRadianceCacheParameters;
			passParameters->IrrVolumeParameters = irradianceVolumeParameters;

			auto shader = ShaderMap::Get<FinalGatherCS>();
			ComputeShaderUtils::AddPass<FinalGatherCS>(renderGraph,
				"GI.FinalGather",
				shader,
				passParameters,
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
				{ Math::DivideRoundUp(view.width, 8u), Math::DivideRoundUp(view.height, 8u), 1u });
		}

		Output result;
		result.indirectLight = indirectLight;

		return result;
	}

	RGUniformBufferRef GlobalIlluminationRenderer::SetupIrradianceVolumeUniformBuffer(RenderGraph& renderGraph, const RenderView& view)
	{
		RGUniformBufferRef uniformBuffer = renderGraph.CreateUniformBuffer(RGUniformBufferDesc::Create<IrradianceVolumeConstants>("GI.IrradianceVolumeConstants"));

		glm::vec3 cameraPosition = view.camera->GetPosition();

		if (s_giIrradianceVolumeFreezeAtWorldOrigin.GetValue())
		{
			cameraPosition = 0.f;
		}

		if (s_giIrradianceVolumeFreeze.GetValue())
		{
			cameraPosition = m_prevCameraPosition;
		}

		// Set prev camera position to current camera position in the first frame.
		if (view.frameIndex == 0)
		{ 
			m_prevCameraPosition = cameraPosition;
		}

		for (uint32_t i = 0; i < GetNumCascades(); ++i)
		{
			const float cascadeSpacing = s_giIrradianceVolumeSpacing.GetValue() * std::powf(2.f, static_cast<float>(i));

			glm::vec3 origin = cameraPosition;
			origin /= cascadeSpacing;
			origin = glm::floor(origin);
			origin *= cascadeSpacing;

			const glm::vec3 gridMin = origin - cascadeSpacing * (s_giIrradianceVolumeResolution.GetValue() * 0.5f);
			m_irradianceVolumeConstants.cascadeMinCornerAndSpacing[i] = glm::vec4(gridMin, cascadeSpacing);

			{
				const glm::vec3 delta = (cameraPosition - m_prevCameraPosition) / cascadeSpacing;

				glm::ivec3 shift =
				{
					delta.x >= 0.f ? int32_t(glm::floor(delta.x)) : int32_t(glm::ceil(delta.x)),
					delta.y >= 0.f ? int32_t(glm::floor(delta.y)) : int32_t(glm::ceil(delta.y)),
					delta.z >= 0.f ? int32_t(glm::floor(delta.z)) : int32_t(glm::ceil(delta.z)),
				};

				m_irradianceVolumeConstants.cascadeScrollOffset[i].x += shift.x;
				m_irradianceVolumeConstants.cascadeScrollOffset[i].y += shift.y;
				m_irradianceVolumeConstants.cascadeScrollOffset[i].z += shift.z;
			}
		}

		m_prevCameraPosition = cameraPosition;

		AddMappedBufferUploadCopyData(renderGraph, uniformBuffer, &m_irradianceVolumeConstants, sizeof(IrradianceVolumeConstants));

		return uniformBuffer;
	}
}
