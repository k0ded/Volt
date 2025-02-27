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
#include <RenderCore/Shader/ShaderMap.h>

#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	static constexpr uint32_t VolumeSize = 128;

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
			auto pipeline = ShaderMap::GetComputePipeline("VolumetricFogInjectExtinctionScattering");

			context.BindPipeline(pipeline);
			context.SetConstant("rwScatteringExtinction"_sh, data.scatteringExtinctionImage);
			context.SetConstant("viewData"_sh, uniformBuffers.viewDataBuffer);
			context.SetConstant("volumetricFogParams"_sh, volumetricFogParamsBuffer);
			context.SetConstant("heightFogDensity"_sh, 0.1f);
			context.SetConstant("heightFogFalloff"_sh, 1.f);
			context.SetConstant("heightFogColor"_sh, glm::vec3(0.5f, 0.5f, 0.5f));
		
			BlueNoise::Setup(context, blueNoiseTextures);

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
			auto pipeline = ShaderMap::GetComputePipeline("VolumetricFogLightScattering");

			context.BindPipeline(pipeline);
			context.SetConstant("rwLightScattering"_sh, data.lightScatteringImage);
			context.SetConstant("scatteringExtinction"_sh, scatteringExtinctionImage);
			context.SetConstant("viewData"_sh, uniformBuffers.viewDataBuffer);
			context.SetConstant("volumetricFogParams"_sh, volumetricFogParamsBuffer);
			context.SetConstant("directionalLightShadowData"_sh, uniformBuffers.directionalLightShadowDataBuffer);
			context.SetConstant("visibleLights"_sh, lightCullingData.visibleLightsBuffer);
			context.SetConstant("pointSampler"_sh, Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest>()->GetResourceHandle());
			context.SetConstant("phaseAnisotropy"_sh, 0.2f);
			context.SetConstant("lights"_sh, gpuSceneData.lightsBuffer);

			BlueNoise::Setup(context, blueNoiseTextures);

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

		auto& data = renderGraph.AddPass<Data>("Integration",
		[&](RenderGraph::Builder& builder, Data& data)
		{
			data.integrationImage = builder.CreateImage(RGUtils::CreateImage3DDesc<RHI::PixelFormat::R16G16B16A16_SFLOAT>(VolumeSize, VolumeSize, VolumeSize, RHI::ImageUsage::Storage, "VolumetricFog.IntegratedVolume"));

			builder.ReadResource(volumetricFogParamsBuffer);
			builder.ReadResource(uniformBuffers.viewDataBuffer);
			builder.ReadResource(uniformBuffers.directionalLightShadowDataBuffer);
			builder.ReadResource(lightScatteringImage);

			builder.SetIsComputePass();
		},
		[=](const Data& data, RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline("VolumetricFogIntegrate");

			context.BindPipeline(pipeline);
			context.SetConstant("rwIntegratedVolume"_sh, data.integrationImage);
			context.SetConstant("lightScattering"_sh, lightScatteringImage);
			context.SetConstant("viewData"_sh, uniformBuffers.viewDataBuffer);
			context.SetConstant("volumetricFogParams"_sh, volumetricFogParamsBuffer);
			context.SetConstant("pointSampler"_sh, Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest>()->GetResourceHandle());

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
			auto pipeline = ShaderMap::GetComputePipeline("VolumetricFogSpatialFilter");

			context.BindPipeline(pipeline);
			context.SetConstant("rwSpatialFilteredScattering"_sh, data.filteredImage);
			context.SetConstant("lightScattering"_sh, lightScatteringImage);
			context.SetConstant("pointSampler"_sh, Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest>()->GetResourceHandle());
			context.SetConstant("froxelVolumeDimensions"_sh, glm::ivec3(VolumeSize));

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

		renderGraph.AddPass<Data>("TemporalFilter",
		[&](RenderGraph::Builder& builder, Data& data) 
		{
			data.prevLightScatteringImage = m_previousLightScatteringImage ? builder.AddExternalImage(m_previousLightScatteringImage) : builder.AddExternalImage(Renderer::GetDefaultResources().black1x1x1);

			builder.WriteResource(lightScatteringImage);
			builder.ReadResource(data.prevLightScatteringImage);
			builder.ReadResource(uniformBuffers.viewDataBuffer);
			builder.ReadResource(volumetricFogParamsBuffer);

			builder.SetIsComputePass();
		},
		[=](const Data& data, RenderContext& context) 
		{
			auto pipeline = ShaderMap::GetComputePipeline("VolumetricFogTemporalFilter");

			context.BindPipeline(pipeline);
			context.SetConstant("rwLightScattering"_sh, lightScatteringImage);
			context.SetConstant("prevLightScattering"_sh, data.prevLightScatteringImage);
			context.SetConstant("viewData"_sh, uniformBuffers.viewDataBuffer);
			context.SetConstant("volumetricFogParams"_sh, volumetricFogParamsBuffer);
			context.SetConstant("alpha"_sh, 0.3f);
			context.SetConstant("pointSampler"_sh, Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest>()->GetResourceHandle());

			context.Dispatch(Math::DivideRoundUp(VolumeSize, 8u), Math::DivideRoundUp(VolumeSize, 8u), VolumeSize);
		});

		renderGraph.EnqueueImageExtraction(lightScatteringImage, m_previousLightScatteringImage);
	}
}
