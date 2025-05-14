#include "sbpch.h"

#include "Sandbox/SceneRendererExtensions/OutlineTechnique.h"

#include <Volt-Renderer/RenderingTechniques/CullingTechnique.h>
#include <Volt-Renderer/SceneRendererStructs.h>
#include <Volt-Renderer/RendererCommon.h>
#include <Volt-Renderer/RenderScene.h>
#include <Volt-Renderer/Renderer.h>

#include <RenderCore/RenderGraph/ShaderParameterStruct.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/ShaderMap.h>

using namespace Volt;

struct OutlineGeometryMSPS
{
	BEGIN_SHADER_DEFINITION(OutlineGeometryMSPS)
		DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/AmplificationCommon.hlsl", "MainAS", RHI::ShaderStage::Amplification)
	DECLARE_SHADER_STAGE("Engine/Shaders/Source/Editor/Outline/OutlineGeometry.hlsl", "MainMS", RHI::ShaderStage::Mesh)
	DECLARE_SHADER_STAGE("Engine/Shaders/Source/Editor/Outline/OutlineGeometry.hlsl", "MainPS", RHI::ShaderStage::Pixel)
	END_SHADER_DEFINITION()

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(MeshShaderCommonParameters, Common)
	END_SHADER_PARAMETER_STRUCT()
};
REGISTER_SHADER(OutlineGeometryMSPS);

struct JumpFloodInitVSPS
{
	BEGIN_SHADER_DEFINITION(JumpFloodInitVSPS)
		DECLARE_SHADER_STAGE("Engine/Shaders/Source/Utility/FullscreenTriangle_vs.hlsl", "main", RHI::ShaderStage::Vertex)
	DECLARE_SHADER_STAGE("Engine/Shaders/Source/Editor/Outline/JumpFlood.hlsl", "JumpFloodInitPS", RHI::ShaderStage::Pixel)
	END_SHADER_DEFINITION()

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_IMAGE(vt::Tex2D<float4>, InputColor)
		SHADER_PARAMETER_SAMPLER(vt::TextureSampler, PointSampler)
		SHADER_PARAMETER(float2, RenderSize)
	END_SHADER_PARAMETER_STRUCT()
};
REGISTER_SHADER(JumpFloodInitVSPS);

struct JumpFloodPassVSPS
{
	BEGIN_SHADER_DEFINITION(JumpFloodPassVSPS)
		DECLARE_SHADER_STAGE("Engine/Shaders/Source/Editor/Outline/JumpFlood.hlsl", "JumpFloodPassVS", RHI::ShaderStage::Vertex)
	DECLARE_SHADER_STAGE("Engine/Shaders/Source/Editor/Outline/JumpFlood.hlsl", "JumpFloodPassPS", RHI::ShaderStage::Pixel)
	END_SHADER_DEFINITION()

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_IMAGE(vt::Tex2D<float4>, InputColor)
		SHADER_PARAMETER_SAMPLER(vt::TextureSampler, PointSampler)
		SHADER_PARAMETER(float2, TexelSize)
		SHADER_PARAMETER(int32_t, Step)
	END_SHADER_PARAMETER_STRUCT()
};
REGISTER_SHADER(JumpFloodPassVSPS);

struct OutlineCompositeCS
{
	BEGIN_SHADER_DEFINITION(OutlineCompositeCS)
		DECLARE_SHADER_STAGE("Engine/Shaders/Source/Editor/Outline/OutlineComposite.hlsl", "OutlineCompositeCS", RHI::ShaderStage::Compute)
	END_SHADER_DEFINITION()

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_IMAGE(vt::RWTex2D<float4>, RWOutputColor)
		SHADER_PARAMETER_IMAGE(vt::Tex2D<float4>, JumpFloodResult)
		SHADER_PARAMETER(float3, OutlineColor)
		SHADER_PARAMETER(uint2, RenderSize)
	END_SHADER_PARAMETER_STRUCT()
};
REGISTER_SHADER(OutlineCompositeCS);

OutlineTechnique::OutlineTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	: m_renderGraph(renderGraph), m_blackboard(blackboard)
{
}

void OutlineTechnique::Execute(RenderGraphBufferHandle selectedPrimitivesMask, RenderGraphImageHandle dstImage, RenderScene& renderScene)
{
	const auto& viewData = m_blackboard.Get<ViewUniformBuffer>();

	DrawCullingData cullingData;

	// Perform culling
	{
		CullingTechnique cullingTechnique{ m_renderGraph, m_blackboard };
		CullingTechnique::Info info{};
		info.viewMatrix = viewData.view;
		info.cullingFrustum = viewData.cullingFrustum;
		info.nearPlane = viewData.nearPlane;
		info.farPlane = viewData.farPlane;
		info.drawCommandCount = renderScene.GetDrawCount();
		info.meshletCount = renderScene.GetMeshletCount();
		info.entityMaskBuffer = selectedPrimitivesMask;

		cullingData = cullingTechnique.Execute(info);
	}

	RenderGraphImageHandle outlineGeometryImage = AddDrawOutlineGeometryPass(cullingData);
	RenderGraphImageHandle jumpFloodImage = AddJumpFloodInitPass(outlineGeometryImage);

	const int32_t numSteps = 2;
	int32_t step = (int32_t)std::round(std::pow(numSteps - 1, 2));

	while (step != 0)
	{
		jumpFloodImage = AddJumpFloodPass(jumpFloodImage, step);
		step /= 2;
	}

	AddOutlineCompositePass(dstImage, jumpFloodImage);
}

RenderGraphImageHandle OutlineTechnique::AddDrawOutlineGeometryPass(const DrawCullingData& cullingData)
{
	struct Data
	{
		RenderGraphImageHandle color;
		RenderGraphImageHandle depth;
	};

	const auto& viewData = m_blackboard.Get<ViewUniformBuffer>();
	const auto& uniformBuffers = m_blackboard.Get<UniformBuffersData>();
	const auto& gpuSceneData = m_blackboard.Get<GPUSceneData>();

	Data& data = m_renderGraph.AddPass<Data>("Outline GeometryPass",
	[&](RenderGraph::Builder& builder, Data& data)
	{
		data.color = builder.CreateImage(RGUtils::CreateImage2DDesc<RHI::PixelFormat::R8G8B8A8_UNORM>(viewData.renderSize.x, viewData.renderSize.y, RHI::ImageUsage::AttachmentStorage, "OutlineGeometryColor"));
		data.depth = builder.CreateImage(RGUtils::CreateImage2DDesc<RHI::PixelFormat::D32_SFLOAT>(viewData.renderSize.x, viewData.renderSize.y, RHI::ImageUsage::AttachmentStorage, "OutlineGeometryDepth"));

		BuildGPUSceneData(builder, gpuSceneData);

		builder.ReadResource(uniformBuffers.viewDataBuffer);
		builder.ReadResource(cullingData.countCommandBuffer, RenderGraphResourceState::IndirectArgument);
		builder.ReadResource(cullingData.taskCommandsBuffer);
	},
	[=](const Data& data, RenderContext& context)
	{
		RenderingInfo info = context.CreateRenderingInfo(viewData.renderSize.x, viewData.renderSize.y, { data.color, data.depth });

		RHI::RenderPipelineCreateInfo pipelineInfo{};
		pipelineInfo.shader = ShaderMap::Get<OutlineGeometryMSPS>();

		auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

		context.BeginRendering(info);
		context.BindPipeline(pipeline);

		OutlineGeometryMSPS::Parameters parameters;
		parameters.Common.GPUSceneData = gpuSceneData;
		parameters.Common.TaskCommands = cullingData.taskCommandsBuffer;
		parameters.Common.View = uniformBuffers.viewDataBuffer;

		context.SetParameters<OutlineGeometryMSPS>(parameters);
		context.DispatchMeshTasksIndirect(cullingData.countCommandBuffer, sizeof(uint32_t), 1, 0);
		context.EndRendering();
	});

	return data.color;
}

RenderGraphImageHandle OutlineTechnique::AddJumpFloodInitPass(RenderGraphImageHandle outlineGeometryImage)
{
	struct Data
	{
		RenderGraphImageHandle color;
	};

	const auto& viewData = m_blackboard.Get<ViewUniformBuffer>();

	Data& data = m_renderGraph.AddPass<Data>("JumpFloodInitPass",
	[&](RenderGraph::Builder& builder, Data& data)
	{
		data.color = builder.CreateImage(RGUtils::CreateImage2DDesc<RHI::PixelFormat::R16G16B16A16_SFLOAT>(viewData.renderSize.x, viewData.renderSize.y, RHI::ImageUsage::AttachmentStorage, "JumpFloodInit"));
		builder.ReadResource(outlineGeometryImage);
	},
	[=](const Data& data, RenderContext& context)
	{
		RenderingInfo info = context.CreateRenderingInfo(viewData.renderSize.x, viewData.renderSize.y, { data.color });

		RHI::RenderPipelineCreateInfo pipelineInfo{};
		pipelineInfo.shader = ShaderMap::Get<JumpFloodInitVSPS>();

		auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

		JumpFloodInitVSPS::Parameters parameters;
		parameters.InputColor = outlineGeometryImage;
		parameters.PointSampler = Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest>()->GetResourceHandle();
		parameters.RenderSize = viewData.renderSize;

		context.BeginRendering(info);
		context.BindPipeline(pipeline);
		context.SetParameters<JumpFloodInitVSPS>(parameters);
		context.Draw(3, 1, 0, 0);
		context.EndRendering();
	});

	return data.color;
}


RenderGraphImageHandle OutlineTechnique::AddJumpFloodPass(RenderGraphImageHandle prevImage, int32_t step)
{
	struct Data
	{
		RenderGraphImageHandle color;
	};

	const auto& viewData = m_blackboard.Get<ViewUniformBuffer>();

	Data& data = m_renderGraph.AddPass<Data>("JumpFloodPass",
	[&](RenderGraph::Builder& builder, Data& data)
	{
		data.color = builder.CreateImage(RGUtils::CreateImage2DDesc<RHI::PixelFormat::R16G16B16A16_SFLOAT>(viewData.renderSize.x, viewData.renderSize.y, RHI::ImageUsage::AttachmentStorage, "JumpFloodImage"));
		builder.ReadResource(prevImage);
	},
	[=](const Data& data, RenderContext& context)
	{
		RenderingInfo info = context.CreateRenderingInfo(viewData.renderSize.x, viewData.renderSize.y, { data.color });

		RHI::RenderPipelineCreateInfo pipelineInfo{};
		pipelineInfo.shader = ShaderMap::Get<JumpFloodPassVSPS>();

		auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

		JumpFloodPassVSPS::Parameters parameters;
		parameters.InputColor = prevImage;
		parameters.PointSampler = Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest>()->GetResourceHandle();
		parameters.TexelSize = 1.f / glm::vec2(viewData.renderSize);
		parameters.Step = step;

		context.BeginRendering(info);
		context.BindPipeline(pipeline);
		context.SetParameters<JumpFloodPassVSPS>(parameters);
		context.Draw(3, 1, 0, 0);
		context.EndRendering();
	});

	return data.color;
}

void OutlineTechnique::AddOutlineCompositePass(RenderGraphImageHandle dstImage, RenderGraphImageHandle jumpfloodOutput)
{
	const auto& viewData = m_blackboard.Get<ViewUniformBuffer>();

	m_renderGraph.AddPass("OutlineCompositePass",
	[&](RenderGraph::Builder& builder)
	{
		builder.ReadResource(jumpfloodOutput);
		builder.WriteResource(dstImage);
	},
	[=](RenderContext& context)
	{
		auto pipeline = ShaderMap::GetComputePipeline<OutlineCompositeCS>();

		context.BindPipeline(pipeline);

		OutlineCompositeCS::Parameters parameters;
		parameters.RWOutputColor = dstImage;
		parameters.JumpFloodResult = jumpfloodOutput;
		parameters.RenderSize = viewData.renderSize;
		parameters.OutlineColor = glm::vec3(1.f, 0.5f, 0.f);

		context.SetParameters<OutlineCompositeCS>(parameters);
		context.Dispatch(Math::DivideRoundUp(viewData.renderSize.x, 8u), Math::DivideRoundUp(viewData.renderSize.y, 8u), 1u);
	});
}
