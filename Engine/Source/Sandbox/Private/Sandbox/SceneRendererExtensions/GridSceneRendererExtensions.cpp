#include "sbpch.h"
#include "Sandbox/SceneRendererExtensions/GridSceneRendererExtension.h"

#include <Volt-Renderer/SceneRendererStructs.h>
#include <Volt-Renderer/SceneRendererRenderGraphData.h>
#include <Volt-Renderer/RenderView.h>
#include <Volt-Renderer/RendererCommon.h>
#include <Volt-Renderer/Camera/Camera.h>

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderContext.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/ShaderParameterStruct.h>
#include <RenderCore/RenderGraph/ShaderRegistry.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/PipelineStateCache.h>
#include <RenderCore/DefaultBlendStates.h>

using namespace Volt;

struct EditorGridVS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(EditorGridVS)

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
		SHADER_PARAMETER(float4x4, NonReversedInverseProjection)
	END_SHADER_PARAMETER_STRUCT()
};
REGISTER_SHADER(EditorGridVS, "Engine/Shaders/Source/Editor/3DGrid.hlsl", "GridVS", Vertex);

struct EditorGridPS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(EditorGridPS)

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
		RG_RENDER_TARGETS()
	END_SHADER_PARAMETER_STRUCT()
};
REGISTER_SHADER(EditorGridPS, "Engine/Shaders/Source/Editor/3DGrid.hlsl", "GridPS", Pixel);

BEGIN_SHADER_PARAMETER_STRUCT(EditorGridParameters)
	SHADER_PARAMETER_STRUCT_INCLUDE(EditorGridVS::Parameters, VS)
	SHADER_PARAMETER_STRUCT_INCLUDE(EditorGridPS::Parameters, PS)
END_SHADER_PARAMETER_STRUCT()

RGTextureRef GridSceneRendererExtension::OnRender(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, const Volt::RenderView& view, Volt::RGTextureRef prevOutputImage)
{
	const Volt::SceneTextures& sceneTextures = blackboard.Get<Volt::SceneTextures>();

	EditorGridParameters* passParameters = renderGraph.AllocParameters<EditorGridParameters>();
	passParameters->VS.View = view.viewUniformBuffer;
	passParameters->VS.NonReversedInverseProjection = glm::inverse(view.camera->GetNonReversedProjection());
	passParameters->PS.View = view.viewUniformBuffer;
	passParameters->PS.renderTargets.renderTargets[0] = prevOutputImage;
	passParameters->PS.renderTargets.depthTarget = sceneTextures.sceneDepth;

	auto vertexShader = ShaderMap::Get<EditorGridVS>();
	auto pixelShader = ShaderMap::Get<EditorGridPS>();

	renderGraph.AddPass("Editor Grid",
		RenderGraphPassFlags::None,
		passParameters,
		[passParameters, view, vertexShader, pixelShader](RenderContext& context) 
	{
		RenderingInfo info = context.CreateRenderingInfo(view.width, view.height, passParameters->PS.renderTargets);
		info.renderingInfo.colorAttachments[0].clearMode = RHI::ClearMode::Load;
		info.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;

		RHI::RenderPipelineCreateInfo pipelineInfo;
		pipelineInfo.shaders = { vertexShader, pixelShader };
		pipelineInfo.attachmentBlendStates[0] = DefaultBlendStates::Alpha();
		pipelineInfo.depthMode = RHI::DepthMode::Read;

		auto pipeline = PipelineStateCache::GetRenderPipeline(pipelineInfo);

		context.BeginRendering(info);
		context.BindPipeline(pipeline);
		context.SetParameters<EditorGridVS>(vertexShader, &passParameters->VS);
		context.SetParameters<EditorGridPS>(pixelShader, &passParameters->PS);
		context.Draw(3, 1, 0, 0);
		context.EndRendering();
	});

	return prevOutputImage;
}
