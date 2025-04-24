#include "vrpch.h"
#include "Volt-Renderer/RenderingUtils.h"

#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/RenderContextUtils.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/RenderGraph/ShaderParameterStruct.h>
#include <RHIModule/Pipelines/RenderPipeline.h>

namespace Volt::RenderingUtils
{
	struct GenerateIndirectArgsCS
	{
		BEGIN_SHADER_DEFINITION(GenerateIndirectArgsCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Indirect/GenerateIndirectArgs_cs.hlsl", "main", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER(vt::RWTypedBuffer<uint>, RWIndirectArgs)
			SHADER_PARAMETER_BUFFER(vt::TypedBuffer<uint>, CountBuffer)
			SHADER_PARAMETER(uint, ThreadGroupSize)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(GenerateIndirectArgsCS)

	struct GenerateIndirectArgsWrappedCS
	{
		BEGIN_SHADER_DEFINITION(GenerateIndirectArgsWrappedCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Indirect/GenerateIndirectArgsWrapped_cs.hlsl", "main", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER(vt::RWTypedBuffer<uint>, RWIndirectArgs)
			SHADER_PARAMETER_BUFFER(vt::TypedBuffer<uint>, CountBuffer)
			SHADER_PARAMETER(uint, GroupSize)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(GenerateIndirectArgsWrappedCS)

	struct CopyImageVSPS
	{
		BEGIN_SHADER_DEFINITION(CopyImageVSPS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Utility/FullscreenTriangle_vs.hlsl", "main", RHI::ShaderStage::Vertex)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Utility/CopyImage_ps.hlsl", "main", RHI::ShaderStage::Pixel)
		END_SHADER_DEFINITION()
	
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float3>, Color)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(CopyImageVSPS)

	RenderGraphBufferHandle GenerateIndirectArgs(RenderGraph& renderGraph, RenderGraphBufferHandle countBuffer, uint32_t groupSize, const std::string& argsBufferName)
	{
		struct Output
		{
			RenderGraphBufferHandle argsBufferHandle;
		};

		RenderGraphBufferHandle outHandle;

		renderGraph.AddPass<Output>("Generate Indirect Args",
		[&](RenderGraph::Builder& builder, Output& data)
		{
			const auto argsDesc = RGUtils::CreateBufferDesc<uint32_t>(3, RHI::BufferUsage::StorageBuffer | RHI::BufferUsage::IndirectBuffer, RHI::MemoryUsage::GPU, argsBufferName);
			data.argsBufferHandle = builder.CreateBuffer(argsDesc);
			outHandle = data.argsBufferHandle;

			builder.ReadResource(countBuffer);
			builder.SetIsComputePass();
		},
		[=](const Output& data, RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline<GenerateIndirectArgsCS>();

			GenerateIndirectArgsCS::Parameters parameters;
			parameters.RWIndirectArgs = data.argsBufferHandle;
			parameters.CountBuffer = countBuffer;
			parameters.ThreadGroupSize = groupSize;

			context.BindPipeline(pipeline);
			context.SetParameters<GenerateIndirectArgsCS>(parameters);
			context.Dispatch(1, 1, 1);
		});

		return outHandle;
	}

	RenderGraphBufferHandle GenerateIndirectArgsWrapped(RenderGraph& renderGraph, RenderGraphBufferHandle countBuffer, uint32_t groupSize, const std::string& argsBufferName)
	{
		struct Output
		{
			RenderGraphBufferHandle argsBufferHandle;
		};

		auto& data = renderGraph.AddPass<Output>("Generate Indirect Args",
		[&](RenderGraph::Builder& builder, Output& data)
		{
			const auto argsDesc = RGUtils::CreateBufferDesc<uint32_t>(3, RHI::BufferUsage::StorageBuffer | RHI::BufferUsage::IndirectBuffer, RHI::MemoryUsage::GPU, argsBufferName);
			data.argsBufferHandle = builder.CreateBuffer(argsDesc);

			builder.ReadResource(countBuffer);
			builder.SetIsComputePass();
		},
		[=](const Output& data, RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline<GenerateIndirectArgsWrappedCS>();

			GenerateIndirectArgsWrappedCS::Parameters parameters;
			parameters.RWIndirectArgs = data.argsBufferHandle;
			parameters.CountBuffer = countBuffer;
			parameters.GroupSize = groupSize;

			context.BindPipeline(pipeline);
			context.SetParameters<GenerateIndirectArgsWrappedCS>(parameters);
			context.Dispatch(1, 1, 1);
		});

		return data.argsBufferHandle;
	}

	void CopyImage(RenderGraph& renderGraph, RenderGraphImageHandle sourceImage, RenderGraphImageHandle destinationImage, const glm::uvec2& renderSize)
	{
		renderGraph.AddPass("Copy Image",
		[&](RenderGraph::Builder& builder) 
		{
			builder.ReadResource(sourceImage);
			builder.WriteResource(destinationImage);
		},
		[=](RenderContext& context)
		{
			RenderingInfo info = context.CreateRenderingInfo(renderSize.x, renderSize.y, { destinationImage });

			RHI::RenderPipelineCreateInfo pipelineInfo;
			pipelineInfo.shader = ShaderMap::Get<CopyImageVSPS>();
			pipelineInfo.depthMode = RHI::DepthMode::None;
			auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);
		
			context.BeginRendering(info);

			RCUtils::DrawFullscreenTriangle(context, pipeline, [&](RenderContext& context)
			{
				context.SetConstant("color"_sh, sourceImage);
			});

			context.EndRendering();
		});
	}
}
