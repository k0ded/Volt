#include "sbpch.h"

#include "Sandbox/SceneRendererExtensions/DebugSceneRendererExtension.h"
#include "Sandbox/SceneRendererExtensions/ObjectIDSceneRendererExtension.h"
#include "Sandbox/DebugMeshRenderers/DebugMeshRenderers.h"

#include <Volt-Renderer/SceneRendererRenderGraphData.h>
#include <Volt-Renderer/Renderer.h>
#include <Volt-Renderer/Debug/DebugRenderer.h>
#include <Volt-Renderer/RenderView.h>
#include <Volt-Renderer/RenderScene.h>
#include <Volt-Renderer/SceneRenderer.h>
#include <Volt-Renderer/Material/MaterialShaderRegistry.h>
#include <Volt-Renderer/RenderingTechniques/CascadedShadowMapsTechnique.h>

#include <RenderCore/RenderGraph/ShaderRegistry.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/BatchedShaderParameters.h>
#include <RenderCore/SamplerStateCache.h>
#include <RenderCore/Shader/DefaultShaders.h>
#include <RenderCore/DefaultBlendStates.h>

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

BEGIN_SHADER_PARAMETER_STRUCT(TranslucencyDebugMeshParameters)
	SHADER_PARAMETER_STRUCT_INCLUDE(ForwardLitDebugVS::Parameters, VS)
	SHADER_PARAMETER_STRUCT_INCLUDE(TranslucencyDebugMaterialShader::Parameters, PS)
	RG_RENDER_TARGETS()
END_SHADER_PARAMETER_STRUCT()

DebugSceneRendererExtension::DebugSceneRendererExtension(Ref<RenderScene> renderScene, DebugRenderer& debugRenderer)
	: SceneRendererExtension(renderScene),
	m_debugRenderer(debugRenderer)
{
	m_debugRenderer.AddDebugMeshRenderer<ForwardLitDebugMeshRenderer>();
	m_debugRenderer.AddDebugMeshRenderer<TranslucencyDebugMeshRenderer>();
}

RGTextureRef DebugSceneRendererExtension::OnRender(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view, RGTextureRef prevOutputImage)
{
	renderGraph.BeginMarker("Debug Rendering");

	const SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();
	const ObjectIDTexture& objectIdTexture = blackboard.Get<ObjectIDTexture>();

	m_rgVisProxyTexture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R32_UINT>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "Debug.VisProxyID"));
	m_depthTexture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::D32_SFLOAT>(view.width, view.height, RHI::ImageUsage::Attachment, "Debug.Depth"));

	AddClearUAVPass(renderGraph, renderGraph.CreateUAV(m_rgVisProxyTexture), glm::uvec4{ 0xFFFFFFFF });
	AddCopyTexturePass(renderGraph, sceneTextures.sceneDepth, m_depthTexture);

	ShaderParameterRenderTargetBindings renderTargets;
	renderTargets.renderTargets[0] = prevOutputImage;
	renderTargets.renderTargets[1] = objectIdTexture.texture;
	renderTargets.renderTargets[2] = m_rgVisProxyTexture;
	renderTargets.depthTarget = m_depthTexture;

	{
		auto pixelShader = ShaderMap::Get<EditorGizmoPS>();
		m_debugRenderer.RenderBillboards(renderGraph, pixelShader, view, renderTargets, false);
	}

	m_debugRenderer.PrepareMeshesForRendering(renderGraph);

	RenderForwardLitDebugMeshes(renderGraph, blackboard, view, prevOutputImage);
	RenderTranslucentDebugMeshes(renderGraph, blackboard, view, prevOutputImage);

	renderGraph.EnqueueTextureExtraction(m_rgVisProxyTexture, &m_visProxyIdImage);
	renderGraph.EndMarker();

	return prevOutputImage;
}

void DebugSceneRendererExtension::RenderForwardLitDebugMeshes(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, const Volt::RenderView& view, Volt::RGTextureRef prevOutputImage)
{
	ForwardLitDebugMeshRenderer* forwardLitDebugMeshRenderer = m_debugRenderer.GetDebugMeshRenderer<ForwardLitDebugMeshRenderer>();

	if (forwardLitDebugMeshRenderer->HasAnyDraw())
	{
		const EnvironmentTextures& environmentTextures = blackboard.Get<EnvironmentTextures>();
		const SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();
		const ObjectIDTexture& objectIdTexture = blackboard.Get<ObjectIDTexture>();
		const CascadedShadowMapsTechnique::Result directionalShadowMap = blackboard.Get<CascadedShadowMapsTechnique::Result>();
		const LightScene& lightScene = blackboard.Get<LightScene>();

		ForwardLitDebugMeshParameters* passParameters = renderGraph.AllocParameters<ForwardLitDebugMeshParameters>();
		passParameters->VS.View = view.viewUniformBuffer;
		passParameters->VS.DebugMeshDatas = renderGraph.CreateSRV(forwardLitDebugMeshRenderer->GetDebugMeshDataBuffer());
		passParameters->VS.PrimitiveIndexVertexBuffer = forwardLitDebugMeshRenderer->GetPrimitiveIndexBuffer();

		passParameters->PS.SceneDepth = renderGraph.CreateSRV(sceneTextures.sceneDepth);

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
		
		passParameters->renderTargets.renderTargets[0] = prevOutputImage;
		passParameters->renderTargets.renderTargets[1] = objectIdTexture.texture;
		passParameters->renderTargets.renderTargets[2] = m_rgVisProxyTexture;
		passParameters->renderTargets.depthTarget = m_depthTexture;

		renderGraph.AddPass("ForwardLit",
			RenderGraphPassFlags::None,
			passParameters,
			[passParameters, view, forwardLitDebugMeshRenderer](RenderContext& context)
		{
			BatchedShaderParameters batchedShaderParameters;
			context.CollectParameters(passParameters, batchedShaderParameters);

			RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->renderTargets);
			renderingInfo.renderingInfo.colorAttachments[0].clearMode = RHI::ClearMode::Load;
			renderingInfo.renderingInfo.colorAttachments[1].clearMode = RHI::ClearMode::Load;
			renderingInfo.renderingInfo.colorAttachments[2].clearMode = RHI::ClearMode::Load;
			renderingInfo.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;

			context.BeginRendering(renderingInfo);
			forwardLitDebugMeshRenderer->ExecuteCommands(context, batchedShaderParameters);
			context.EndRendering();
		});
	}
}

void DebugSceneRendererExtension::RenderTranslucentDebugMeshes(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, const Volt::RenderView& view, Volt::RGTextureRef prevOutputImage)
{
	TranslucencyDebugMeshRenderer* translucencyDebugMeshRenderer = m_debugRenderer.GetDebugMeshRenderer<TranslucencyDebugMeshRenderer>();
	if (translucencyDebugMeshRenderer->HasAnyDraw())
	{
		const EnvironmentTextures& environmentTextures = blackboard.Get<EnvironmentTextures>();
		const SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();
		const ObjectIDTexture& objectIdTexture = blackboard.Get<ObjectIDTexture>();
		const CascadedShadowMapsTechnique::Result directionalShadowMap = blackboard.Get<CascadedShadowMapsTechnique::Result>();
		const LightScene& lightScene = blackboard.Get<LightScene>();

		RGTextureRef accumulation = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "Debug.Translucency.Accumulation"));
		RGTextureRef revealage = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R8_UNORM>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "Debug.Translucency.Revealage"));

		// Render the meshes
		{
			TranslucencyDebugMeshParameters* passParameters = renderGraph.AllocParameters<TranslucencyDebugMeshParameters>();
			passParameters->VS.View = view.viewUniformBuffer;
			passParameters->VS.DebugMeshDatas = renderGraph.CreateSRV(translucencyDebugMeshRenderer->GetDebugMeshDataBuffer());
			passParameters->VS.PrimitiveIndexVertexBuffer = translucencyDebugMeshRenderer->GetPrimitiveIndexBuffer();

			passParameters->PS.SceneDepth = renderGraph.CreateSRV(sceneTextures.sceneDepth);

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

			passParameters->renderTargets.renderTargets[0] = accumulation;
			passParameters->renderTargets.renderTargets[1] = revealage;
			passParameters->renderTargets.renderTargets[2] = objectIdTexture.texture;
			passParameters->renderTargets.renderTargets[3] = m_rgVisProxyTexture;
			passParameters->renderTargets.depthTarget = m_depthTexture;

			renderGraph.AddPass("Translucency",
			RenderGraphPassFlags::None,
			passParameters,
			[passParameters, view, translucencyDebugMeshRenderer](RenderContext& context)
			{
				BatchedShaderParameters batchedShaderParameters;
				context.CollectParameters(passParameters, batchedShaderParameters);

				RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->renderTargets);
				renderingInfo.renderingInfo.colorAttachments[1].SetClearColor(1.f, 1.f, 1.f, 1.f);
				renderingInfo.renderingInfo.colorAttachments[2].clearMode = RHI::ClearMode::Load;
				renderingInfo.renderingInfo.colorAttachments[3].clearMode = RHI::ClearMode::Load;
				renderingInfo.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;

				context.BeginRendering(renderingInfo);
				translucencyDebugMeshRenderer->ExecuteCommands(context, batchedShaderParameters);
				context.EndRendering();
			});
		}

		// Composite
		{
			TranslucencyCompositePS::Parameters* passParameters = renderGraph.AllocParameters<TranslucencyCompositePS::Parameters>();
			passParameters->Accumulation = renderGraph.CreateSRV(accumulation);
			passParameters->Revealage = renderGraph.CreateSRV(revealage);
			passParameters->renderTargets.renderTargets[0] = prevOutputImage;

			auto vertexShader = ShaderMap::Get<FullscreenTriangleVS>();
			auto pixelShader = ShaderMap::Get<TranslucencyCompositePS>();

			renderGraph.AddPass("TranslucencyComposite",
				RenderGraphPassFlags::None,
				passParameters,
				[passParameters, view, pixelShader, vertexShader](RenderContext& context)
			{
				GraphicsPipelineState pipelineState{};
				pipelineState.shaders = { vertexShader, pixelShader };
				pipelineState.cullMode = RHI::CullMode::None;
				pipelineState.depthMode = RHI::DepthMode::None;
				pipelineState.attachmentBlendStates[0] = DefaultBlendStates::OneMinusSrcAlpha();
				pipelineState.renderTargets = passParameters->renderTargets;

				RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->renderTargets);
				renderingInfo.renderingInfo.colorAttachments[0].clearMode = RHI::ClearMode::Load;

				context.BeginRendering(renderingInfo);
				context.SetPipelineState(pipelineState);
				context.SetParameters<TranslucencyCompositePS>(pixelShader, passParameters);
				context.Draw(3, 1, 0, 0);
				context.EndRendering();
			});
		}
	}
}
