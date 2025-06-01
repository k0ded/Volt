#include "sbpch.h"
#include "Sandbox/SceneRendererExtensions/GridSceneRendererExtension.h"

#include <Volt-Renderer/SceneRendererStructs.h>
#include <Volt-Renderer/RendererCommon.h>
#include <Volt-Renderer/Camera/Camera.h>

#include <RenderCore/RenderGraph2/RenderGraph2.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph2/ShaderParameterStruct2.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/Shader/ShaderMap.h>
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

RGTextureRef GridSceneRendererExtension::OnRender(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, Ref<Volt::Camera> camera, Volt::RGTextureRef prevOutputImage)
{
#if 0
	const auto& depthPrePass = blackboard.Get<DepthPrePass>();
	const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();
	const auto& viewUniformBuffer = blackboard.Get<ViewUniformBuffer>();

	renderGraph.AddPass("Editor Grid Pass",
	[&](RenderGraph::Builder& builder) 
	{
		builder.WriteResource(prevOutputImage);
		builder.WriteResource(depthPrePass.depth);
		builder.ReadResource(uniformBuffers.viewDataBuffer);
	},
	[=](RenderContext& context)
	{
		RenderingInfo info = context.CreateRenderingInfo(viewUniformBuffer.renderSize.x, viewUniformBuffer.renderSize.y, { prevOutputImage, depthPrePass.depth });
		info.renderingInfo.colorAttachments[0].clearMode = RHI::ClearMode::Load;
		info.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;

		RHI::RenderPipelineCreateInfo pipelineInfo;
		pipelineInfo.shader = ShaderMap::Get<EditorGridVSPS>();
		pipelineInfo.attachmentBlendStates[0] = DefaultBlendStates::Alpha();

		auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

		EditorGridVSPS::Parameters parameters;
		parameters.View = uniformBuffers.viewDataBuffer;
		parameters.NonReversedInverseProjection = glm::inverse(camera->GetNonReversedProjection());

		context.BeginRendering(info);
		context.BindPipeline(pipeline);
		context.SetParameters<EditorGridVSPS>(parameters);
		context.Draw(3, 1, 0, 0);
		context.EndRendering();
	});
#endif

	return prevOutputImage;
}
