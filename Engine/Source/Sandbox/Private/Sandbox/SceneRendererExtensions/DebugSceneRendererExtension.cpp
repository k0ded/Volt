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
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/BatchedShaderParameters.h>
#include <RenderCore/SamplerStateCache.h>

struct EditorGizmoPS : public Volt::GlobalShader
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

DebugSceneRendererExtension::DebugSceneRendererExtension(Ref<Volt::RenderScene> renderScene, Volt::DebugRenderer& debugRenderer)
	: Volt::SceneRendererExtension(renderScene),
	m_debugRenderer(debugRenderer)
{
	m_debugRenderer.AddDebugMeshRenderer<ForwardLitDebugMeshRenderer>();
}

Volt::RGTextureRef DebugSceneRendererExtension::OnRender(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, const Volt::RenderView& view, Volt::RGTextureRef prevOutputImage)
{
	const Volt::SceneTextures& sceneTextures = blackboard.Get<Volt::SceneTextures>();
	const ObjectIDTexture& objectIdTexture = blackboard.Get<ObjectIDTexture>();


	Volt::ShaderParameterRenderTargetBindings renderTargets;
	renderTargets.renderTargets[0] = prevOutputImage;
	renderTargets.renderTargets[1] = objectIdTexture.texture;
	renderTargets.depthTarget = sceneTextures.sceneDepth;

	{
		auto pixelShader = Volt::ShaderMap::Get<EditorGizmoPS>();
		m_debugRenderer.RenderBillboards(renderGraph, pixelShader, view, renderTargets, false);
	}

	m_debugRenderer.PrepareMeshesForRendering(renderGraph);

	{
		const Volt::EnvironmentTextures& environmentTextures = blackboard.Get<Volt::EnvironmentTextures>();
		const Volt::CascadedShadowMapsTechnique::Result directionalShadowMap = blackboard.Get<Volt::CascadedShadowMapsTechnique::Result>();
		const Volt::LightScene& lightScene = blackboard.Get<Volt::LightScene>();

		ForwardLitDebugMeshRenderer* forwardLitDebugMeshRenderer = m_debugRenderer.GetDebugMeshRenderer<ForwardLitDebugMeshRenderer>();

		ForwardLitDebugMeshParameters* passParameters = renderGraph.AllocParameters<ForwardLitDebugMeshParameters>();
		passParameters->VS.View = view.viewUniformBuffer;
		passParameters->VS.DebugMeshDatas = renderGraph.CreateSRV(forwardLitDebugMeshRenderer->GetDebugMeshDataBuffer());
		passParameters->VS.PrimitiveIndexVertexBuffer = forwardLitDebugMeshRenderer->GetPrimitiveIndexBuffer();

		passParameters->PS.View = view.viewUniformBuffer;
		passParameters->PS.GPUScene = m_renderScene->GetGPUSceneParameters(renderGraph);
		passParameters->PS.VisibleLightIndices = renderGraph.CreateSRV(lightScene.visibleLightIndices, Volt::RHI::PixelFormat::R32_SINT);
		passParameters->PS.DFGLuT = renderGraph.CreateSRV(environmentTextures.DFGLuT);
		passParameters->PS.SkylightIrradiance = renderGraph.CreateSRV(environmentTextures.irradiance);
		passParameters->PS.SkylightRadiance = renderGraph.CreateSRV(environmentTextures.radiance);
		passParameters->PS.LinearSampler = Volt::SamplerStateCache::GetSampler<Volt::RHI::TextureFilter::Linear, Volt::RHI::TextureFilter::Linear, Volt::RHI::TextureFilter::Linear, Volt::RHI::TextureWrap::Clamp>();
		passParameters->PS.NumRadianceMipLevels = environmentTextures.radiance->GetDesc().mips;

		Volt::RGTextureRef directionalShadowMapTexture = directionalShadowMap.shadowMap;

		if (!directionalShadowMapTexture)
		{
			directionalShadowMapTexture = renderGraph.RegisterExternalTexture(Volt::Renderer::GetDefaultResources().blackCubeTexture);
		}

		passParameters->PS.CascadedDirectionalShadowMap = renderGraph.CreateSRV(directionalShadowMapTexture);
		passParameters->PS.ShadowSampler = Volt::SamplerStateCache::GetSampler<Volt::RHI::TextureFilter::Linear, Volt::RHI::TextureFilter::Linear, Volt::RHI::TextureFilter::Linear, Volt::RHI::TextureWrap::Repeat, Volt::RHI::AnisotropyLevel::None, Volt::RHI::CompareOperator::LessEqual>();
		passParameters->PS.CascadedDirectionalLightShadowMapping = directionalShadowMap.uniformBuffer;
		passParameters->renderTargets = renderTargets;

		renderGraph.AddPass("Forward Lit Debug Meshes",
			Volt::RenderGraphPassFlags::None,
			passParameters,
			[passParameters, view, forwardLitDebugMeshRenderer](Volt::RenderContext& context) 
		{
			Volt::BatchedShaderParameters batchedShaderParameters;
			context.CollectParameters(passParameters, batchedShaderParameters);

			Volt::RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->renderTargets);
			renderingInfo.renderingInfo.colorAttachments[0].clearMode = Volt::RHI::ClearMode::Load;
			renderingInfo.renderingInfo.colorAttachments[1].clearMode = Volt::RHI::ClearMode::Load;
			renderingInfo.renderingInfo.depthAttachmentInfo.clearMode = Volt::RHI::ClearMode::Load;

			context.BeginRendering(renderingInfo);
			forwardLitDebugMeshRenderer->ExecuteCommands(context, batchedShaderParameters);
			context.EndRendering();
		});
	}

	return prevOutputImage;
}
