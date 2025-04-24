#include "vrpch.h"

#include "Volt-Renderer/RenderingTechniques/VolumetricFogTechnique.h"
#include "Volt-Renderer/RenderingTechniques/LightCullingTechnique.h"
#include "Volt-Renderer/SceneRendererStructs.h"
#include "Volt-Renderer/RendererCommon.h"
#include "Volt-Renderer/BlueNoise.h"
#include "Volt-Renderer/Renderer.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	static constexpr uint32_t VolumeSize = 128;

	struct VolumetricFogInjectExtinctionScatteringCS
	{
		BEGIN_SHADER_DEFINITION(VolumetricFogInjectExtinctionScatteringCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Volumetrics/Fog/VolumetricFogInjectExtinctionScattering.hlsl", "MainCS", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::RWTex3D<float4>, RWScatteringExtinction)
			SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<ViewData>, View)
			SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<VolumetricFogParams>, VolumetricFogParamsData)
			SHADER_PARAMETER(float, HeightFogDensity)
			SHADER_PARAMETER(float, HeightFogFalloff)
			SHADER_PARAMETER(float3, HeightFogColor)
			SHADER_PARAMETER_STRUCT(BlueNoiseShaderParameters, BlueNoise)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(VolumetricFogInjectExtinctionScatteringCS)

	struct VolumetricFogLightScatteringCS
	{
		BEGIN_SHADER_DEFINITION(VolumetricFogLightScatteringCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Volumetrics/Fog/VolumetricFogLightScattering.hlsl", "MainCS", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::RWTex3D<float4>, RWLightScattering)
			SHADER_PARAMETER_IMAGE(vt::Tex3D<float4>, ScatteringExtinction)
			SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<ViewData>, View)
			SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<VolumetricFogParams>, VolumetricFogParamsData)
			SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<DirectionalLightShadowData>, DirectionalLightShadow)
			SHADER_PARAMETER_BUFFER(vt::TypedBuffer<LightDrawData>, Lights)
			SHADER_PARAMETER_BUFFER(vt::TypedBuffer<int>, VisibleLights)
			SHADER_PARAMETER_SAMPLER(vt::TextureSampler, PointSampler)
			SHADER_PARAMETER_STRUCT(BlueNoiseShaderParameters, BlueNoise)
			SHADER_PARAMETER(float, PhaseAnisotropy)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(VolumetricFogLightScatteringCS)

	struct VolumetricFogIntegrateCS
	{
		BEGIN_SHADER_DEFINITION(VolumetricFogLightScatteringCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Volumetrics/Fog/VolumetricFogIntegrate.hlsl", "MainCS", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::RWTex3D<float4>, RWIntegratedVolume)
			SHADER_PARAMETER_IMAGE(vt::Tex3D<float4>, LightScattering)
			SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<ViewData>, View)
			SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<VolumetricFogParams>, VolumetricFogParamsData)
			SHADER_PARAMETER_SAMPLER(vt::TextureSampler, PointSampler)
			SHADER_PARAMETER_STRUCT(BlueNoiseShaderParameters, BlueNoise)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(VolumetricFogIntegrateCS)

	struct VolumetricFogSpatialFilterCS
	{
		BEGIN_SHADER_DEFINITION(VolumetricFogSpatialFilterCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Volumetrics/Fog/VolumetricFogSpatialFilter.hlsl", "MainCS", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::RWTex3D<float4>, RWSpatialFilteredScattering)
			SHADER_PARAMETER_IMAGE(vt::Tex3D<float4>, LightScattering)
			SHADER_PARAMETER_SAMPLER(vt::TextureSampler, PointSampler)
			SHADER_PARAMETER(int3, FroxelVolumeDimensions)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(VolumetricFogSpatialFilterCS)

	struct VolumetricFogTemporalFilterCS
	{
		BEGIN_SHADER_DEFINITION(VolumetricFogTemporalFilterCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Volumetrics/Fog/VolumetricFogTemporalFilter.hlsl", "MainCS", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::RWTex3D<float4>, RWLightScattering)
			SHADER_PARAMETER_IMAGE(vt::Tex3D<float4>, PrevLightScattering)
			SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<ViewData>, View)
			SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<VolumetricFogParams>, VolumetricFogParamsData)
			SHADER_PARAMETER_SAMPLER(vt::TextureSampler, PointSampler)
			SHADER_PARAMETER(float, Alpha)
			SHADER_PARAMETER_STRUCT(BlueNoiseShaderParameters, BlueNoise)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(VolumetricFogTemporalFilterCS)

	struct VolumetricFogParams
	{
		float temporalReprojectionJitterScale;
		float froxelNearPlane;
		float froxelFarPlane;
		float scatteringFactor;
		glm::ivec3 froxelVolumeDimensions;
	};

	VolumetricFogData VolumetricFogTechnique::Execute(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		renderGraph.BeginMarker("VolumetricFog", glm::vec4(0.3f, 0.5f, 0.f, 1.f));

		RenderGraphUniformBufferHandle volumetricFogParamsBuffer = renderGraph.CreateUniformBuffer(RGUtils::CreateBufferDesc<VolumetricFogParams>(1, RHI::BufferUsage::StorageBuffer, RHI::MemoryUsage::CPUToGPU, "VolumetricFog.VolumetricFogParams"));
		
		{
			const auto& viewUniformBuffer = blackboard.Get<ViewUniformBuffer>();

			VolumetricFogParams fogParams;
			fogParams.temporalReprojectionJitterScale = 0.2f;
			fogParams.scatteringFactor = 0.1f;
			fogParams.froxelVolumeDimensions = 128;
			fogParams.froxelNearPlane = viewUniformBuffer.nearPlane;
			fogParams.froxelFarPlane = viewUniformBuffer.farPlane;

			renderGraph.AddMappedBufferUpload(volumetricFogParamsBuffer, &fogParams, sizeof(VolumetricFogParams), "VolumetricFog.UploadFogParams");
		}


		VolumetricFogData fogData;
		fogData.fogParamsBuffer = volumetricFogParamsBuffer;

		RenderGraphImageHandle scatteringExtinctionImage = ExecuteInjectExtinctionScattering(renderGraph, blackboard, volumetricFogParamsBuffer);
		RenderGraphImageHandle lightScatteringImage = ExecuteLightScattering(renderGraph, blackboard, volumetricFogParamsBuffer, scatteringExtinctionImage);
		RenderGraphImageHandle spatiallyFilteredImage = ExecuteSpatialFilter(renderGraph, blackboard, lightScatteringImage);
		ExecuteTemporalFilter(renderGraph, blackboard, volumetricFogParamsBuffer, lightScatteringImage);
		fogData.integratedFogVolume = ExecuteIntegration(renderGraph, blackboard, volumetricFogParamsBuffer, spatiallyFilteredImage);

		renderGraph.EndMarker();

		return fogData;
	}

	RenderGraphImageHandle VolumetricFogTechnique::ExecuteInjectExtinctionScattering(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphUniformBufferHandle volumetricFogParamsBuffer)
	{
		struct Data
		{
			RenderGraphImageHandle scatteringExtinctionImage = RenderGraphNullHandle();
		};

		const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();
		const auto& blueNoiseTextures = blackboard.Get<BlueNoiseTextures>();

		auto& data = renderGraph.AddPass<Data>("InjectExtinctionScattering",
		[&](RenderGraph::Builder& builder, Data& data) 
		{
			data.scatteringExtinctionImage = builder.CreateImage(RGUtils::CreateImage3DDesc<RHI::PixelFormat::R16G16B16A16_SFLOAT>(VolumeSize, VolumeSize, VolumeSize, RHI::ImageUsage::Storage, "VolumetricFog.ScatteringExtinction"));
		
			builder.ReadResource(volumetricFogParamsBuffer);
			builder.ReadResource(uniformBuffers.viewDataBuffer);

			BlueNoise::Build(builder, blueNoiseTextures);

			builder.SetIsComputePass();
		}, 
		[=](const Data& data, RenderContext& context) 
		{
			auto pipeline = ShaderMap::GetComputePipeline<VolumetricFogInjectExtinctionScatteringCS>();

			VolumetricFogInjectExtinctionScatteringCS::Parameters parameters;
			parameters.RWScatteringExtinction = data.scatteringExtinctionImage;
			parameters.View = uniformBuffers.viewDataBuffer;
			parameters.VolumetricFogParamsData = volumetricFogParamsBuffer;
			parameters.HeightFogDensity = 0.1f;
			parameters.HeightFogFalloff = 1.f;
			parameters.HeightFogColor = 0.5f;

			BlueNoise::Setup(parameters.BlueNoise, blueNoiseTextures);

			context.BindPipeline(pipeline);
			context.SetParameters<VolumetricFogInjectExtinctionScatteringCS>(parameters);

			context.Dispatch(Math::DivideRoundUp(VolumeSize, 8u), Math::DivideRoundUp(VolumeSize, 8u), VolumeSize);
		});

		return data.scatteringExtinctionImage;
	}

	RenderGraphImageHandle VolumetricFogTechnique::ExecuteLightScattering(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphUniformBufferHandle volumetricFogParamsBuffer, RenderGraphImageHandle scatteringExtinctionImage)
	{
		struct Data
		{
			RenderGraphImageHandle lightScatteringImage = RenderGraphNullHandle();
		};

		const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();
		const auto& blueNoiseTextures = blackboard.Get<BlueNoiseTextures>();
		const auto& gpuSceneData = blackboard.Get< GPUSceneData>();
		const auto& lightCullingData = blackboard.Get<LightCullingData>();

		auto& data = renderGraph.AddPass<Data>("LightScattering",
		[&](RenderGraph::Builder& builder, Data& data) 
		{
			data.lightScatteringImage = builder.CreateImage(RGUtils::CreateImage3DDesc<RHI::PixelFormat::R16G16B16A16_SFLOAT>(VolumeSize, VolumeSize, VolumeSize, RHI::ImageUsage::Storage, "VolumetricFog.LightScattering"));

			builder.ReadResource(volumetricFogParamsBuffer);
			builder.ReadResource(uniformBuffers.viewDataBuffer);
			builder.ReadResource(scatteringExtinctionImage);
			builder.ReadResource(uniformBuffers.directionalLightShadowDataBuffer);
			builder.ReadResource(lightCullingData.visibleLightsBuffer);
			builder.ReadResource(gpuSceneData.lightsBuffer);

			BlueNoise::Build(builder, blueNoiseTextures);

			builder.SetIsComputePass();
		},
		[=](const Data& data, RenderContext& context) 
		{
			auto pipeline = ShaderMap::GetComputePipeline<VolumetricFogLightScatteringCS>();

			VolumetricFogLightScatteringCS::Parameters parameters;
			parameters.RWLightScattering = data.lightScatteringImage;
			parameters.ScatteringExtinction = scatteringExtinctionImage;
			parameters.View = uniformBuffers.viewDataBuffer;
			parameters.VolumetricFogParamsData = volumetricFogParamsBuffer;
			parameters.DirectionalLightShadow = uniformBuffers.directionalLightShadowDataBuffer;
			parameters.Lights = gpuSceneData.lightsBuffer;
			parameters.VisibleLights = lightCullingData.visibleLightsBuffer;
			parameters.PointSampler = Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest>()->GetResourceHandle();
			parameters.PhaseAnisotropy = 0.2f;

			BlueNoise::Setup(parameters.BlueNoise, blueNoiseTextures);

			context.BindPipeline(pipeline);
			context.SetParameters<VolumetricFogLightScatteringCS>(parameters);
			context.Dispatch(Math::DivideRoundUp(VolumeSize, 8u), Math::DivideRoundUp(VolumeSize, 8u), VolumeSize);
		});

		return data.lightScatteringImage;
	}

	RenderGraphImageHandle VolumetricFogTechnique::ExecuteIntegration(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphUniformBufferHandle volumetricFogParamsBuffer, RenderGraphImageHandle lightScatteringImage)
	{
		struct Data
		{
			RenderGraphImageHandle integrationImage = RenderGraphNullHandle();
		};

		const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();
		const auto& blueNoiseTextures = blackboard.Get<BlueNoiseTextures>();

		auto& data = renderGraph.AddPass<Data>("Integration",
		[&](RenderGraph::Builder& builder, Data& data)
		{
			data.integrationImage = builder.CreateImage(RGUtils::CreateImage3DDesc<RHI::PixelFormat::R16G16B16A16_SFLOAT>(VolumeSize, VolumeSize, VolumeSize, RHI::ImageUsage::Storage, "VolumetricFog.IntegratedVolume"));

			builder.ReadResource(volumetricFogParamsBuffer);
			builder.ReadResource(uniformBuffers.viewDataBuffer);
			builder.ReadResource(uniformBuffers.directionalLightShadowDataBuffer);
			builder.ReadResource(lightScatteringImage);

			BlueNoise::Build(builder, blueNoiseTextures);

			builder.SetIsComputePass();
		},
		[=](const Data& data, RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline<VolumetricFogIntegrateCS>();

			VolumetricFogIntegrateCS::Parameters parameters;
			parameters.RWIntegratedVolume = data.integrationImage;
			parameters.LightScattering = lightScatteringImage;
			parameters.View = uniformBuffers.viewDataBuffer;
			parameters.VolumetricFogParamsData = volumetricFogParamsBuffer;
			parameters.PointSampler = Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest>()->GetResourceHandle();

			BlueNoise::Setup(parameters.BlueNoise, blueNoiseTextures);

			context.BindPipeline(pipeline);
			context.SetParameters<VolumetricFogIntegrateCS>(parameters);
			context.Dispatch(Math::DivideRoundUp(VolumeSize, 8u), Math::DivideRoundUp(VolumeSize, 8u), 1);
		});
	
		return data.integrationImage;
	}

	RenderGraphImageHandle VolumetricFogTechnique::ExecuteSpatialFilter(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphImageHandle lightScatteringImage)
	{
		struct Data
		{
			RenderGraphImageHandle filteredImage = RenderGraphNullHandle();
		};

		auto& data = renderGraph.AddPass<Data>("SpatialFilter",
		[&](RenderGraph::Builder& builder, Data& data)
		{
			data.filteredImage = builder.CreateImage(RGUtils::CreateImage3DDesc<RHI::PixelFormat::R16G16B16A16_SFLOAT>(VolumeSize, VolumeSize, VolumeSize, RHI::ImageUsage::Storage, "VolumetricFog.SpatialFiltered"));
			
			builder.ReadResource(lightScatteringImage);
			builder.SetIsComputePass();
		},
		[=](const Data& data, RenderContext& context) 
		{
			auto pipeline = ShaderMap::GetComputePipeline<VolumetricFogSpatialFilterCS>();

			VolumetricFogSpatialFilterCS::Parameters parameters;
			parameters.RWSpatialFilteredScattering = data.filteredImage;
			parameters.LightScattering = lightScatteringImage;
			parameters.PointSampler = Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest>()->GetResourceHandle();
			parameters.FroxelVolumeDimensions = VolumeSize;

			context.BindPipeline(pipeline);
			context.SetParameters<VolumetricFogSpatialFilterCS>(parameters);
			context.Dispatch(Math::DivideRoundUp(VolumeSize, 8u), Math::DivideRoundUp(VolumeSize, 8u), VolumeSize);
		});

		return data.filteredImage;
	}

	void VolumetricFogTechnique::ExecuteTemporalFilter(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphUniformBufferHandle volumetricFogParamsBuffer, RenderGraphImageHandle lightScatteringImage)
	{
		struct Data
		{
			RenderGraphImageHandle prevLightScatteringImage;
		};

		const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();
		const auto& blueNoiseTextures = blackboard.Get<BlueNoiseTextures>();

		renderGraph.AddPass<Data>("TemporalFilter",
		[&](RenderGraph::Builder& builder, Data& data) 
		{
			data.prevLightScatteringImage = m_previousLightScatteringImage ? builder.AddExternalImage(m_previousLightScatteringImage) : builder.AddExternalImage(Renderer::GetDefaultResources().black1x1x1);

			builder.WriteResource(lightScatteringImage);
			builder.ReadResource(data.prevLightScatteringImage);
			builder.ReadResource(uniformBuffers.viewDataBuffer);
			builder.ReadResource(volumetricFogParamsBuffer);

			BlueNoise::Build(builder, blueNoiseTextures);

			builder.SetIsComputePass();
		},
		[=](const Data& data, RenderContext& context) 
		{
			auto pipeline = ShaderMap::GetComputePipeline<VolumetricFogTemporalFilterCS>();

			VolumetricFogTemporalFilterCS::Parameters parameters;
			parameters.RWLightScattering = lightScatteringImage;
			parameters.PrevLightScattering = data.prevLightScatteringImage;
			parameters.View = uniformBuffers.viewDataBuffer;
			parameters.VolumetricFogParamsData = volumetricFogParamsBuffer;
			parameters.Alpha = 0.3f;
			parameters.PointSampler = Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest>()->GetResourceHandle();

			BlueNoise::Setup(parameters.BlueNoise, blueNoiseTextures);

			context.BindPipeline(pipeline);
			context.SetParameters<VolumetricFogTemporalFilterCS>(parameters);
			context.Dispatch(Math::DivideRoundUp(VolumeSize, 8u), Math::DivideRoundUp(VolumeSize, 8u), VolumeSize);
		});

		renderGraph.EnqueueImageExtraction(lightScatteringImage, m_previousLightScatteringImage);
	}
}
