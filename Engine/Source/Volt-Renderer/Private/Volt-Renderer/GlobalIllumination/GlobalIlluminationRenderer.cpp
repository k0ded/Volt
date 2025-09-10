#include "vrpch.h"
#include "Volt-Renderer/GlobalIllumination/GlobalIlluminationRenderer.h"
#include "Volt-Renderer/RenderingTechniques/CascadedDirectionalShadowTechnique.h"
#include "Volt-Renderer/RenderView.h"
#include "Volt-Renderer/GPUScene.h"
#include "Volt-Renderer/BlueNoise.h"
#include "Volt-Renderer/SceneRendererRenderGraphData.h"
#include "Volt-Renderer/RenderScene.h"
#include "Volt-Renderer/RayTracing/RayTracingScene.h"
#include "Volt-Renderer/Renderer.h"

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
			SHADER_PARAMETER_ACCELERATION_STRUCTURE(TLAS)
			SHADER_PARAMETER_RAY_TRACING_RESOURCE_TABLE(RTResources)
			SHADER_PARAMETER_STRUCT_INCLUDE(GPUSceneParameters, GPUScene)
			SHADER_PARAMETER_STRUCT_INCLUDE(BlueNoiseShaderParameters, BlueNoise)

			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, DFGLuT)
			SHADER_PARAMETER_TEXTURE_SRV(TextureCube<float3>, SkylightIrradiance)
			SHADER_PARAMETER_TEXTURE_SRV(TextureCube<float3>, SkylightRadiance)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2DArray<float>, CascadedDirectionalShadowMap)
			SHADER_PARAMETER_UNIFORM_BUFFER(CascadedDirectionalLightShadowMappingData, CascadedDirectionalLightShadowMapping)
			SHADER_PARAMETER_SAMPLER(LinearSampler)
			SHADER_PARAMETER_SAMPLER(ShadowSampler)
			SHADER_PARAMETER(uint, NumRadianceMipLevels)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(FinalGatherCS, "Engine/Shaders/Source/GlobalIllumination/FinalGather.hlsl", "FinalGatherCS", Compute);

	GlobalIlluminationRenderer::Output GlobalIlluminationRenderer::Execute(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view)
	{
		const EnvironmentTextures& environmentTextures = blackboard.Get<EnvironmentTextures>();
		const CascadedDirectionalShadowTechnique::Result& directionalShadowMap = blackboard.Get<CascadedDirectionalShadowTechnique::Result>();

		SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();

		RGTextureRef indirectLight = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "GI.IndirectLight"));

		FinalGatherCS::Parameters* passParameters = renderGraph.AllocParameters<FinalGatherCS::Parameters>();
		passParameters->View = view.viewUniformBuffer;
		passParameters->RWIndirectLight = renderGraph.CreateUAV(indirectLight);
		passParameters->GBufferAlbedo = renderGraph.CreateSRV(sceneTextures.gBufferAlbedo);
		passParameters->GBufferNormal = renderGraph.CreateSRV(sceneTextures.gBufferNormals);
		passParameters->GBufferMaterial = renderGraph.CreateSRV(sceneTextures.gBufferMaterial);
		passParameters->SceneDepth = renderGraph.CreateSRV(sceneTextures.sceneDepth);
		passParameters->TLAS = view.renderScene->GetRayTracingScene()->GetAccelerationStructure();
		passParameters->RTResources = view.renderScene->GetRayTracingResourceTable();
		passParameters->GPUScene = view.renderScene->GetGPUSceneParameters(renderGraph);
		passParameters->BlueNoise = BlueNoise::GetBlueNoiseParameters(renderGraph);

		passParameters->DFGLuT = renderGraph.CreateSRV(environmentTextures.DFGLuT);
		passParameters->SkylightIrradiance = renderGraph.CreateSRV(environmentTextures.irradiance);
		passParameters->SkylightRadiance = renderGraph.CreateSRV(environmentTextures.radiance);
		passParameters->LinearSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Clamp>();
		passParameters->NumRadianceMipLevels = environmentTextures.radiance->GetDesc().mips;

		RGTextureRef directionalShadowTexture = directionalShadowMap.shadowMap;

		if (!directionalShadowTexture)
		{
			directionalShadowTexture = renderGraph.RegisterExternalTexture(Renderer::GetDefaultResources().blackCubeTexture);
		}

		passParameters->CascadedDirectionalShadowMap = renderGraph.CreateSRV(directionalShadowTexture);
		passParameters->ShadowSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Repeat, RHI::AnisotropyLevel::None, RHI::CompareOperator::LessEqual>();
		passParameters->CascadedDirectionalLightShadowMapping = directionalShadowMap.uniformBuffer;

		auto shader = ShaderMap::Get<FinalGatherCS>();
		ComputeShaderUtils::AddPass<FinalGatherCS>(renderGraph,
			"GI.FinalGather",
			shader,
			passParameters,
			RenderGraphPassFlags::NeverCull,
			{ Math::DivideRoundUp(view.width, 8u), Math::DivideRoundUp(view.height, 8u), 1u });

		Output result;
		result.indirectLight = indirectLight;
		
		return result;
	}
}
