#include "vrpch.h"
#include "Volt-Renderer/RenderingTechniques/PrefixSumTechnique.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>

#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	struct PrefixSumCS
	{
		BEGIN_SHADER_DEFINITION(PrefixSumCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Utility/PrefixSum.hlsl", "main", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER(vt::TypedBuffer<uint>, InputValues)
			SHADER_PARAMETER_BUFFER(vt::RWTypedBuffer<uint>, OutputValues)
			SHADER_PARAMETER_BUFFER(vt::RWTypedBuffer<StateBuffer>, StateBuffer)
			SHADER_PARAMETER_BUFFER(vt::RWRawByteBuffer, CounterBuffer)
			SHADER_PARAMETER(uint32_t, ValueCount)
		END_SHADER_PARAMETER_STRUCT()
	};

	REGISTER_SHADER(PrefixSumCS)

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

		auto pipeline = ShaderMap::GetComputePipeline<PrefixSumCS>();

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

			PrefixSumCS::Parameters parameters;
			parameters.InputValues = inputBuffer;
			parameters.OutputValues = outputBuffer;
			parameters.StateBuffer = stateBuffer;
			parameters.CounterBuffer = counterBuffer;
			parameters.ValueCount = valueCount;

			context.SetParameters<PrefixSumCS>(parameters);
			context.Dispatch(groupCount, 1, 1);
		});
	}
}
