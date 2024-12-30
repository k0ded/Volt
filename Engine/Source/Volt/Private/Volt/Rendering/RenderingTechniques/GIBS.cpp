#include "vtpch.h"

#include "Volt/Rendering/RenderingTechniques/GIBS.h"
#include "Volt/Rendering/RenderingTechniques/PrefixSumTechnique.h"
#include "Volt/Rendering/RendererCommon.h"
#include "Volt/Rendering/SceneRendererStructs.h"
#include "Volt/Rendering/Camera/Camera.h"
#include "Volt/Rendering/RenderingUtils.h"
#include "Volt/Math/Math.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <RHIModule/Buffers/StorageBuffer.h>

namespace Volt
{
	struct Surfel
	{
		glm::vec3 worldPosition;
		float radius;
		glm::vec3 normal;
		float padding;
	};

	struct SurfelGridCell
	{
		uint32_t surfelCount;
		uint32_t cellIndirectOffset;
	};

	inline static constexpr uint32_t MaxSurfelCount = 150000;
	inline static constexpr float SurfelCellSize = 100.f;
	inline static constexpr uint32_t SurfelGridSize = 100;

	void GIBS::Render(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, uint32_t frameIndex)
	{
		AllocateBuffers();

		const auto& viewData = blackboard.Get<ViewUniformBuffer>();
		//const auto& renderData = blackboard.Get<RenderData>();
		const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();
		const auto& gbufferData = blackboard.Get<GBufferData>();
		const auto& preDepthData = blackboard.Get<DepthPrePass>();

		const uint32_t renderWidth = static_cast<uint32_t>(viewData.renderSize.x);
		const uint32_t renderHeight = static_cast<uint32_t>(viewData.renderSize.y);
		const bool shouldReset = frameIndex == 0;

		RenderGraphBufferHandle surfelsBufferHandle = renderGraph.AddExternalBuffer(m_surfelsBuffer);
		RenderGraphBufferHandle surfelAllocatorBuffer = renderGraph.AddExternalBuffer(m_surfelAllocatorBuffer);
		RenderGraphBufferHandle allocatedSurfelsBuffer = renderGraph.AddExternalBuffer(m_allocatedSurfelsBuffer);
		RenderGraphImageHandle surfelCoverage = renderGraph.CreateImage(RGUtils::CreateImage2DDesc<RHI::PixelFormat::R32_SFLOAT>(renderWidth, renderHeight, RHI::ImageUsage::Storage, "GIBS.SurfelCoverage"));
		RenderGraphImageHandle debugTexture = renderGraph.CreateImage(RGUtils::CreateImage2DDesc<RHI::PixelFormat::R32G32B32A32_SFLOAT>(renderWidth, renderHeight, RHI::ImageUsage::Storage, "GIBS.DebugTexture"));
		RenderGraphBufferHandle surfelCellCountsBuffer = renderGraph.CreateBuffer(RGUtils::CreateBufferDescGPU<uint32_t>(SurfelGridSize * SurfelGridSize, "GIBS.SurfelCellCounts"));

		renderGraph.AddPass("GIBS.InitializeSurfelAllocator",
		[&](RenderGraph::Builder& builder)
		{
			builder.WriteResource(surfelAllocatorBuffer);
		},
		[=](RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline("InitializeSurfelAllocator");

			context.BindPipeline(pipeline);
			context.SetConstant("rwSurfelAllocator"_sh, surfelAllocatorBuffer);
			context.SetConstant("numMaxSurfels"_sh, MaxSurfelCount);
			context.SetConstant("reset"_sh, shouldReset);

			context.Dispatch(1, 1, 1);
		});

		RenderGraphBufferHandle generateSurfelCountsIndirectArgs = RenderingUtils::GenerateIndirectArgs(renderGraph, surfelAllocatorBuffer, 64u, "GIBS.GenerateSurfelCountArgs");

		RGUtils::ClearBuffer(renderGraph, surfelCellCountsBuffer, 0);

		renderGraph.AddPass("GIBS.GenerateSurfelCounts",
		[&](RenderGraph::Builder& builder)
		{
			builder.WriteResource(surfelCellCountsBuffer);
			builder.ReadResource(surfelAllocatorBuffer);
			builder.ReadResource(surfelsBufferHandle);
			builder.ReadResource(allocatedSurfelsBuffer);
			builder.ReadResource(generateSurfelCountsIndirectArgs, RenderGraphResourceState::IndirectArgument);
			
			builder.SetIsComputePass();
			builder.SetHasSideEffect();
		},
		[=](RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline("GenerateSurfelCellCounts");

			context.BindPipeline(pipeline);
			context.SetConstant("surfelCellCounts"_sh, surfelCellCountsBuffer);
			context.SetConstant("surfels"_sh, surfelsBufferHandle);
			context.SetConstant("surfelAllocator"_sh, surfelAllocatorBuffer);
			context.SetConstant("allocatedSurfels"_sh, allocatedSurfelsBuffer);

			context.DispatchIndirect(generateSurfelCountsIndirectArgs, 0);
		});

		RenderGraphBufferHandle surfelCellPrefixSums = renderGraph.CreateBuffer(RGUtils::CreateBufferDescGPU<uint32_t>(SurfelGridSize * SurfelGridSize, "GIBS.SurfelCellPrefixSums"));

		PrefixSumTechnique surfelIndirectionPrefixSum(renderGraph);
		surfelIndirectionPrefixSum.Execute(surfelCellCountsBuffer, surfelCellPrefixSums, MaxSurfelCount);

		RGUtils::ClearBuffer(renderGraph, surfelCellCountsBuffer, 0);

		RenderGraphBufferHandle surfelGridIndirections = renderGraph.CreateBuffer(RGUtils::CreateBufferDescGPU<uint32_t>(MaxSurfelCount, "GIBS.SurfelGridIndirections"));

		renderGraph.AddPass("GIBS.GenerateSurfelGridIndirection",
		[&](RenderGraph::Builder& builder)
		{
			builder.WriteResource(surfelCellCountsBuffer);
			builder.WriteResource(surfelGridIndirections);
			builder.ReadResource(surfelCellPrefixSums);
			builder.ReadResource(surfelsBufferHandle);
			builder.ReadResource(surfelAllocatorBuffer);
			builder.ReadResource(allocatedSurfelsBuffer);
			builder.ReadResource(generateSurfelCountsIndirectArgs, RenderGraphResourceState::IndirectArgument);
			builder.SetIsComputePass();
			builder.SetHasSideEffect();
		},
		[=](RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline("GenerateSurfelGridIndirection");

			context.BindPipeline(pipeline);
			context.SetConstant("surfelCellIndirections"_sh, surfelGridIndirections);
			context.SetConstant("surfelCellCounts"_sh, surfelCellCountsBuffer);
			context.SetConstant("surfelCellPrefixSums"_sh, surfelCellPrefixSums);
			context.SetConstant("surfels"_sh, surfelsBufferHandle);
			context.SetConstant("surfelAllocator"_sh, surfelAllocatorBuffer);
			context.SetConstant("allocatedSurfels"_sh, allocatedSurfelsBuffer);

			context.DispatchIndirect(generateSurfelCountsIndirectArgs, 0);
		});

		RGUtils::ClearImage(renderGraph, surfelCoverage, { 0.f }, "Clear GIBS.SurfelCoverage");

		renderGraph.AddPass("GIBS.GenerateSurfelCoverage",
		[&](RenderGraph::Builder& builder)
		{
			builder.WriteResource(surfelCoverage);
			builder.WriteResource(debugTexture);
			builder.ReadResource(surfelsBufferHandle);
			builder.ReadResource(surfelAllocatorBuffer);
			builder.ReadResource(uniformBuffers.viewDataBuffer);
			builder.ReadResource(gbufferData.normals);
			builder.ReadResource(preDepthData.depth);
			builder.ReadResource(surfelGridIndirections);
			builder.ReadResource(surfelCellCountsBuffer);
			builder.ReadResource(surfelCellPrefixSums);
			builder.SetIsComputePass();
			builder.SetHasSideEffect();
		},
		[=](RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline("GenerateSurfelCoverage");

			context.BindPipeline(pipeline);
			context.SetConstant("surfelCoverage"_sh, surfelCoverage);
			context.SetConstant("debugTexture"_sh, debugTexture);
			context.SetConstant("surfels"_sh, surfelsBufferHandle);
			context.SetConstant("surfelsAllocator"_sh, surfelAllocatorBuffer);
			context.SetConstant("viewData"_sh, uniformBuffers.viewDataBuffer);
			context.SetConstant("depthTexture"_sh, preDepthData.depth);
			context.SetConstant("normalsTexture"_sh, gbufferData.normals);
			context.SetConstant("surfelCellIndirections"_sh, surfelGridIndirections);
			context.SetConstant("surfelCellCounts"_sh, surfelCellCountsBuffer);
			context.SetConstant("surfelCellStartOffsets"_sh, surfelCellPrefixSums);

			context.Dispatch(Math::DivideRoundUp(static_cast<uint32_t>(viewData.renderSize.x), 16u), Math::DivideRoundUp(static_cast<uint32_t>(viewData.renderSize.y), 16u), 1);
		});

		//if (m_allocate)
		//{
		//	renderGraph.AddPass("GIBS.AllocateSurfelsBasedOnCoverage",
		//	[&](RenderGraph::Builder& builder)
		//	{
		//		builder.WriteResource(surfelsBufferHandle);
		//		builder.WriteResource(surfelsAllocatorBufferHandle);
		//		builder.ReadResource(surfelCoverage);
		//		builder.ReadResource(uniformBuffers.viewDataBuffer);
		//		builder.ReadResource(gbufferData.normals);
		//		builder.ReadResource(preDepthData.depth);
		//		builder.SetIsComputePass();
		//		builder.SetHasSideEffect();
		//	},
		//	[=](RenderContext& context)
		//	{
		//		auto pipeline = ShaderMap::GetComputePipeline("AllocateSurfelsBasedOnCoverage");

		//		context.BindPipeline(pipeline);
		//		context.SetConstant("surfelCoverage"_sh, surfelCoverage);
		//		context.SetConstant("surfels"_sh, surfelsBufferHandle);
		//		context.SetConstant("surfelsAllocator"_sh, surfelsAllocatorBufferHandle);
		//		context.SetConstant("viewData"_sh, uniformBuffers.viewDataBuffer);
		//		context.SetConstant("depthTexture"_sh, preDepthData.depth);
		//		context.SetConstant("normalsTexture"_sh, gbufferData.normals);
		//		context.SetConstant("maxSurfelCount"_sh, MaxSurfelCount);
		//		context.SetConstant("cameraFov"_sh, glm::radians(renderData.camera->GetFieldOfView()));

		//		context.Dispatch(Math::DivideRoundUp(static_cast<uint32_t>(viewData.renderSize.x), 16u), Math::DivideRoundUp(static_cast<uint32_t>(viewData.renderSize.y), 16u), 1);
		//	});

		//	//m_allocate = false;
		//}
	}

	void GIBS::AllocateBuffers()
	{
		constexpr uint32_t SurfelAllocatorBufferCount = 3;

		if (!m_surfelsBuffer)
		{
			m_surfelsBuffer = RHI::StorageBuffer::Create<Surfel>(MaxSurfelCount, "GIBS.SurfelsBuffer");
		}

		if (!m_surfelAllocatorBuffer)
		{
			m_surfelAllocatorBuffer = RHI::StorageBuffer::Create<uint32_t>(SurfelAllocatorBufferCount, "GIBS.SurfelAllocator");
		}

		if (!m_surfelGridBuffer)
		{
			m_surfelGridBuffer = RHI::StorageBuffer::Create<SurfelGridCell>(SurfelGridSize * SurfelGridSize, "GIBS.SurfelGrid");
		}

		if (!m_allocatedSurfelsBuffer)
		{
			m_allocatedSurfelsBuffer = RHI::StorageBuffer::Create<uint32_t>(MaxSurfelCount, "GIBS.AllocatedSurfels");
		}
	}
}
