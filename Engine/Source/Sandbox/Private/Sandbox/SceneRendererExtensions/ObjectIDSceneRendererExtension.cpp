#include "sbpch.h"

#if 0
#include "Sandbox/SceneRendererExtensions/ObjectIDSceneRendererExtension.h"

#include <Volt-Renderer/SceneRendererStructs.h>
#include <Volt-Renderer/RendererCommon.h>

#if 0
#include <Volt-Renderer/RenderingTechniques/CullingTechnique.h>
#endif

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/ShaderParameterStruct.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/ShaderMap.h>

using namespace Volt;

#if 0
struct ObjectIDMSPS
{
	BEGIN_SHADER_DEFINITION(ObjectIDMSPS)
		DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/AmplificationCommon.hlsl", "MainAS", RHI::ShaderStage::Amplification)
		DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/ObjectIDMeshShader.hlsl", "MainMS", RHI::ShaderStage::Mesh)
		DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/ObjectIDMeshShader.hlsl", "MainPS", RHI::ShaderStage::Pixel)
	END_SHADER_DEFINITION()

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(MeshShaderCommonParameters, Common)
	END_SHADER_PARAMETER_STRUCT()
};
REGISTER_SHADER(ObjectIDMSPS);
#endif

Volt::RenderGraphImageHandle ObjectIDSceneRendererExtension::OnRender(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, Ref<Volt::Camera> camera, Volt::RenderGraphImageHandle prevOutputImage)
{
#if 0
	struct Data
	{
		RenderGraphImageHandle objectIdHandle;
	};

	const auto preDepthHandle = blackboard.Get<DepthPrePass>().depth;
	const auto& drawCullingData = blackboard.Get<DrawCullingData>();
	const auto& viewUniformBuffer = blackboard.Get<ViewUniformBuffer>();
	const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();
	const auto& gpuSceneData = blackboard.Get<GPUSceneData>();

	Data& data = renderGraph.AddPass<Data>("Object ID Pass",
	[&](RenderGraph::Builder& builder, Data& data)
	{
		data.objectIdHandle = builder.CreateImage(RGUtils::CreateImage2DDesc<RHI::PixelFormat::R32_UINT>(viewUniformBuffer.renderSize.x, viewUniformBuffer.renderSize.y, RHI::ImageUsage::AttachmentStorage, "Entity ID"));
		builder.WriteResource(preDepthHandle);

		BuildGPUSceneData(builder, gpuSceneData);

		builder.ReadResource(uniformBuffers.viewDataBuffer);
		builder.ReadResource(drawCullingData.countCommandBuffer, RenderGraphResourceState::IndirectArgument);
		builder.ReadResource(drawCullingData.taskCommandsBuffer);

		builder.SetHasSideEffect();
	},
	[=](const Data& data, RenderContext& context)
	{
		RenderingInfo info = context.CreateRenderingInfo(viewUniformBuffer.renderSize.x, viewUniformBuffer.renderSize.y, { data.objectIdHandle, preDepthHandle });
		info.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;

		RHI::RenderPipelineCreateInfo pipelineInfo{};
		pipelineInfo.shader = ShaderMap::Get<ObjectIDMSPS>();
		pipelineInfo.depthCompareOperator = RHI::CompareOperator::Equal;
		pipelineInfo.depthMode = RHI::DepthMode::Read;

		auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

		context.BeginRendering(info);
		context.BindPipeline(pipeline);

		ObjectIDMSPS::Parameters parameters;
		parameters.Common.GPUSceneData = gpuSceneData;
		parameters.Common.TaskCommands = drawCullingData.taskCommandsBuffer;
		parameters.Common.View = uniformBuffers.viewDataBuffer;

		context.SetParameters<ObjectIDMSPS>(parameters);
		context.DispatchMeshTasksIndirect(drawCullingData.countCommandBuffer, sizeof(uint32_t), 1, 0);
		context.EndRendering();
	});

	renderGraph.EnqueueImageExtraction(data.objectIdHandle, m_objectIdImage);

	return prevOutputImage;
#endif
	return {};
}
#endif
