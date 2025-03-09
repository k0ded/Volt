#include "vrpch.h"

#include "Volt-Renderer/RenderingTechniques/CullingTechnique.h"
#include "Volt-Renderer/SceneRendererStructs.h"
#include "Volt-Renderer/RendererCommon.h"

#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	struct DrawCallCullCS
	{
		BEGIN_SHADER_DEFINITION(DrawCallCullCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/DrawCallCull.hlsl", "MainCS", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER(vt::RWTypedBuffer<uint>, CountBuffer)
			SHADER_PARAMETER_BUFFER(vt::RWTypedBuffer<MeshTaskCommand>, TaskCommands)
			SHADER_PARAMETER_STRUCT(GPUSceneData, GPUSceneData)
			SHADER_PARAMETER(glm::mat4, ViewMatrix)
			SHADER_PARAMETER(float4, CullingFrustum)
			SHADER_PARAMETER(float, NearPlane)
			SHADER_PARAMETER(float, FarPlane)
			SHADER_PARAMETER(uint, CullingTypeInt)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(DrawCallCullCS)

	struct TaskSubmitSetupCS
	{
		BEGIN_SHADER_DEFINITION(TaskSubmitSetupCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/TaskSubmitSetup.hlsl", "MainCS", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER(vt::RWTypedBuffer<uint>, CountCommandBuffer)
			SHADER_PARAMETER_BUFFER(vt::RWTypedBuffer<MeshTaskCommand>, TaskCommands)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(TaskSubmitSetupCS)

	CullingTechnique::CullingTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
		: m_renderGraph(renderGraph), m_blackboard(blackboard)
	{
	}

	DrawCullingData CullingTechnique::Execute(const Info& info)
	{
		if (info.drawCommandCount == 0)
		{
			return {};
		}

		m_renderGraph.BeginMarker("Draw Call Culling", { 1.f, 0.f, 0.f, 1.f });

		DrawCullingData data = AddDrawCallCullingPass(info);
		AddTaskSubmitSetupPass(data);

		m_renderGraph.EndMarker();

		return data;
	}

	DrawCullingData CullingTechnique::AddDrawCallCullingPass(const Info& info)
	{
		const auto& gpuSceneData = m_blackboard.Get<GPUSceneData>();

		const auto countCmdBufferDesc = RGUtils::CreateBufferDesc<uint32_t>(4, RHI::BufferUsage::StorageBuffer | RHI::BufferUsage::IndirectBuffer, RHI::MemoryUsage::GPU, "Count And Command Buffer");
		RenderGraphBufferHandle countCmdBufferHandle = m_renderGraph.CreateBuffer(countCmdBufferDesc);

		RGUtils::ClearBuffer(m_renderGraph, countCmdBufferHandle, 0, "Clear Count Buffer");

		DrawCullingData& data = m_renderGraph.AddPass<DrawCullingData>("Draw Call Culling Pass",
		[&](RenderGraph::Builder& builder, DrawCullingData& data) 
		{
			data.countCommandBuffer = countCmdBufferHandle;

			{
				const auto desc = RGUtils::CreateBufferDescGPU<MeshTaskCommand>(Math::DivideRoundUp(info.meshletCount, 32u), "Mesh Task Commands");
				data.taskCommandsBuffer = builder.CreateBuffer(desc);
			}

			BuildGPUSceneData(builder, gpuSceneData);

			builder.WriteResource(countCmdBufferHandle);

			builder.SetIsComputePass();
		},
		[=](const DrawCullingData& data, RenderContext& context) 
		{
			auto pipeline = ShaderMap::GetComputePipeline<DrawCallCullCS>();

			context.BindPipeline(pipeline);

			DrawCallCullCS::Parameters parameters;
			parameters.CountBuffer = data.countCommandBuffer;
			parameters.TaskCommands = data.taskCommandsBuffer;
			parameters.ViewMatrix = info.viewMatrix;
			parameters.CullingFrustum = info.cullingFrustum;
			parameters.NearPlane = info.nearPlane;
			parameters.FarPlane = info.farPlane;
			parameters.CullingTypeInt = static_cast<uint32_t>(info.type);
			parameters.GPUSceneData = gpuSceneData;

			constexpr uint32_t workGroupSize = 64;

			context.SetParameters(parameters);
			context.Dispatch(Math::DivideRoundUp(info.drawCommandCount, workGroupSize), 1, 1);
		});

		return data;
	}

	void CullingTechnique::AddTaskSubmitSetupPass(const DrawCullingData& data)
	{
		m_renderGraph.AddPass("Task Submit Setup Pass",
		[&](RenderGraph::Builder& builder)
		{
			builder.WriteResource(data.countCommandBuffer);
			builder.WriteResource(data.taskCommandsBuffer);
			builder.SetIsComputePass();
		},
		[=](RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline<TaskSubmitSetupCS>();

			TaskSubmitSetupCS::Parameters parameters;
			parameters.CountCommandBuffer = data.countCommandBuffer;
			parameters.TaskCommands = data.taskCommandsBuffer;

			context.BindPipeline(pipeline);
			context.SetParameters(parameters);
			context.Dispatch(1, 1, 1);
		});
	}
}
