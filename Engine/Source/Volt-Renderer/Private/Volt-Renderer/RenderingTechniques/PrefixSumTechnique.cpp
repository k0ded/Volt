#include "vrpch.h"
#include "Volt-Renderer/RenderingTechniques/PrefixSumTechnique.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	PrefixSumTechnique::PrefixSumTechnique(RenderGraph& rg)
		: m_renderGraph(rg)
	{
	}

	void PrefixSumTechnique::Execute(RenderGraphBufferHandle inputBuffer, RenderGraphBufferHandle outputBuffer, const uint32_t valueCount)
	{
		constexpr uint32_t TG_SIZE = 512;

		struct PrefixSumData
		{
			RenderGraphBufferHandle stateBuffer;
		};

		struct State
		{
			uint32_t aggregate;
			uint32_t prefix;
			uint32_t state;
		};

		const uint32_t groupCount = Math::DivideRoundUp(valueCount, TG_SIZE);

		auto pipeline = ShaderMap::GetComputePipeline("PrefixSum");

		RenderGraphBufferHandle counterBuffer = m_renderGraph.CreateBuffer(RGUtils::CreateBufferDescGPU<uint32_t>(1, "PrefixSum.CounterBuffer"));
		RenderGraphBufferHandle stateBuffer = m_renderGraph.CreateBuffer(RGUtils::CreateBufferDescGPU<State>(std::max(groupCount, 1u), "PrefixSum.StateBuffer"));
			
		RGUtils::ClearBuffer(m_renderGraph, stateBuffer, 0);
		RGUtils::ClearBuffer(m_renderGraph, counterBuffer, 0);

		m_renderGraph.AddPass("Prefix Sum",
		[&](RenderGraph::Builder& builder)
		{
			builder.ReadResource(inputBuffer);
			builder.WriteResource(stateBuffer);
			builder.WriteResource(counterBuffer);
			builder.WriteResource(outputBuffer);
			builder.SetIsComputePass();
		},
		[=](RenderContext& context)
		{
			context.BindPipeline(pipeline);
			context.SetConstant("inputValues"_sh, inputBuffer);
			context.SetConstant("outputValues"_sh, outputBuffer);
			context.SetConstant("state"_sh, stateBuffer);
			context.SetConstant("counterBuffer"_sh, counterBuffer);
			context.SetConstant("valueCount"_sh, valueCount);

			context.Dispatch(groupCount, 1, 1);
		});
	}
}
