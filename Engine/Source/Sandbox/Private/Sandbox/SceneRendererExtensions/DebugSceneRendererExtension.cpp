#include "sbpch.h"

#include "Sandbox/SceneRendererExtensions/DebugSceneRendererExtension.h"
#include "Sandbox/SceneRendererExtensions/ObjectIDSceneRendererExtension.h"
#include "Sandbox/DebugMeshRenderers/DebugMeshRenderers.h"

#include <Volt-Renderer/SceneRendererRenderGraphData.h>
#include <Volt-Renderer/Renderer.h>
#include <Volt-Renderer/Debug/DebugRenderer.h>
#include <Volt-Renderer/RenderView.h>
#include <Volt-Renderer/RenderScene.h>
#include <Volt-Renderer/Material/MaterialShaderRegistry.h>
#include <Volt-Renderer/RenderingTechniques/CascadedShadowMapsTechnique.h>

#include <RenderCore/RenderGraph/ShaderRegistry.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/BatchedShaderParameters.h>
#include <RenderCore/SamplerStateCache.h>

using namespace Volt;

struct EditorGizmoPS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(EditorGizmoPS)
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
	END_SHADER_PARAMETER_STRUCT()
};
VT_REGISTER_SHADER(EditorGizmoPS, "Engine/Shaders/Source/Editor/EditorGizmoPS.hlsl", "MainPS", Pixel);

BEGIN_SHADER_PARAMETER_STRUCT(ForwardLitDebugMeshParameters)
	SHADER_PARAMETER_STRUCT_INCLUDE(ForwardLitDebugVS::Parameters, VS)
	SHADER_PARAMETER_STRUCT_INCLUDE(ForwardLitDebugMaterialShader::Parameters, PS)
	RG_RENDER_TARGETS()
END_SHADER_PARAMETER_STRUCT()

DebugSceneRendererExtension::DebugSceneRendererExtension(Ref<RenderScene> renderScene, DebugRenderer& debugRenderer)
	: SceneRendererExtension(renderScene),
	m_debugRenderer(debugRenderer)
{
	m_debugRenderer.AddDebugMeshRenderer<ForwardLitDebugMeshRenderer>();
}

RGTextureRef DebugSceneRendererExtension::OnRender(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view, RGTextureRef prevOutputImage)
{
	const SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();
	const ObjectIDTexture& objectIdTexture = blackboard.Get<ObjectIDTexture>();

	RGTextureRef visProxyIdTexture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R32_UINT>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "HitProxyID"));

	AddClearUAVPass(renderGraph, renderGraph.CreateUAV(visProxyIdTexture), glm::uvec4{ 0u });

	ShaderParameterRenderTargetBindings renderTargets;
	renderTargets.renderTargets[0] = prevOutputImage;
	renderTargets.renderTargets[1] = objectIdTexture.texture;
	renderTargets.renderTargets[2] = visProxyIdTexture;
	renderTargets.depthTarget = sceneTextures.sceneDepth;

	{
		auto pixelShader = ShaderMap::Get<EditorGizmoPS>();
		m_debugRenderer.RenderBillboards(renderGraph, pixelShader, view, renderTargets, false);
	}

	m_debugRenderer.PrepareMeshesForRendering(renderGraph);

	ForwardLitDebugMeshRenderer* forwardLitDebugMeshRenderer = m_debugRenderer.GetDebugMeshRenderer<ForwardLitDebugMeshRenderer>();

	if (forwardLitDebugMeshRenderer->HasAnyDraw())
	{
		const EnvironmentTextures& environmentTextures = blackboard.Get<EnvironmentTextures>();
		const CascadedShadowMapsTechnique::Result directionalShadowMap = blackboard.Get<CascadedShadowMapsTechnique::Result>();
		const LightScene& lightScene = blackboard.Get<LightScene>();


		ForwardLitDebugMeshParameters* passParameters = renderGraph.AllocParameters<ForwardLitDebugMeshParameters>();
		passParameters->VS.View = view.viewUniformBuffer;
		passParameters->VS.DebugMeshDatas = renderGraph.CreateSRV(forwardLitDebugMeshRenderer->GetDebugMeshDataBuffer());
		passParameters->VS.PrimitiveIndexVertexBuffer = forwardLitDebugMeshRenderer->GetPrimitiveIndexBuffer();

		passParameters->PS.View = view.viewUniformBuffer;
		passParameters->PS.GPUScene = m_renderScene->GetGPUSceneParameters(renderGraph);
		passParameters->PS.VisibleLightIndices = renderGraph.CreateSRV(lightScene.visibleLightIndices, RHI::PixelFormat::R32_SINT);
		passParameters->PS.DFGLuT = renderGraph.CreateSRV(environmentTextures.DFGLuT);
		passParameters->PS.SkylightIrradiance = renderGraph.CreateSRV(environmentTextures.irradiance);
		passParameters->PS.SkylightRadiance = renderGraph.CreateSRV(environmentTextures.radiance);
		passParameters->PS.LinearSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Clamp>();
		passParameters->PS.NumRadianceMipLevels = environmentTextures.radiance->GetDesc().mips;

		RGTextureRef directionalShadowMapTexture = directionalShadowMap.shadowMap;

		if (!directionalShadowMapTexture)
		{
			directionalShadowMapTexture = renderGraph.RegisterExternalTexture(Renderer::GetDefaultResources().blackCubeTexture);
		}

		passParameters->PS.CascadedDirectionalShadowMap = renderGraph.CreateSRV(directionalShadowMapTexture);
		passParameters->PS.ShadowSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Repeat, RHI::AnisotropyLevel::None, RHI::CompareOperator::LessEqual>();
		passParameters->PS.CascadedDirectionalLightShadowMapping = directionalShadowMap.uniformBuffer;
		passParameters->renderTargets = renderTargets;

		renderGraph.AddPass("Forward Lit Debug Meshes",
			RenderGraphPassFlags::None,
			passParameters,
			[passParameters, view, forwardLitDebugMeshRenderer](RenderContext& context) 
		{
			BatchedShaderParameters batchedShaderParameters;
			context.CollectParameters(passParameters, batchedShaderParameters);

			RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->renderTargets);
			renderingInfo.renderingInfo.colorAttachments[0].clearMode = RHI::ClearMode::Load;
			renderingInfo.renderingInfo.colorAttachments[1].clearMode = RHI::ClearMode::Load;
			renderingInfo.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;

			context.BeginRendering(renderingInfo);
			forwardLitDebugMeshRenderer->ExecuteCommands(context, batchedShaderParameters);
			context.EndRendering();
		});

		renderGraph.EnqueueTextureExtraction(visProxyIdTexture, &m_visProxyIdImage);
	}

	return prevOutputImage;
}
