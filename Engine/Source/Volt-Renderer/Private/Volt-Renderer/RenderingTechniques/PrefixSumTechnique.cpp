#include "vrpch.h"

#include "Volt-Renderer/RenderingTechniques/PrefixSumTechnique.h"

#include <RenderCore/RenderGraph/ShaderRegistry.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/GlobalShaderMap.h>

#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	struct PrefixSumCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(PrefixSumCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER_SRV(Buffer<uint>, InputValues)
			SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWOutputValues)
			SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWCounterBuffer)
			SHADER_PARAMETER_BUFFER_UAV(RWStructuredBuffer<State>, RWStateBuffer)
			SHADER_PARAMETER(uint, ValueCount)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(PrefixSumCS, "Engine/Shaders/Source/Utility/PrefixSum.hlsl", "MainCS", Compute);

	PrefixSumTechnique::PrefixSumTechnique(RenderGraph& renderGraph)
		: m_renderGraph(renderGraph)
	{
	}

	void PrefixSumTechnique::Execute(RGBufferRef inputBuffer, RGBufferRef outputBuffer, uint32_t numValues)
	{
		constexpr uint32_t TG_SIZE = 512;

		struct State
		{
			uint32_t aggregate;
			uint32_t prefix;
			uint32_t state;
		};

		const uint32_t numGroups = Math::DivideRoundUp(numValues, TG_SIZE);

		RGBufferRef counterBuffer = m_renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(1, "PrefixSum.CounterBuffer"));
		RGBufferRef stateBuffer = m_renderGraph.CreateBuffer(RGBufferDesc::CreateStructuredBufferDesc<State>(numGroups, "PrefixSum.StateBuffer"));

		AddClearUAVPass(m_renderGraph, m_renderGraph.CreateUAV(counterBuffer, RHI::PixelFormat::R32_UINT), 0u);
		AddClearUAVPass(m_renderGraph, m_renderGraph.CreateUAV(stateBuffer), 0u);

		PrefixSumCS::Parameters* passParameters = m_renderGraph.AllocParameters<PrefixSumCS::Parameters>();
		passParameters->InputValues = m_renderGraph.CreateSRV(inputBuffer, RHI::PixelFormat::R32_UINT);
		passParameters->RWOutputValues = m_renderGraph.CreateUAV(outputBuffer, RHI::PixelFormat::R32_UINT);
		passParameters->RWCounterBuffer = m_renderGraph.CreateUAV(counterBuffer, RHI::PixelFormat::R32_UINT);
		passParameters->RWStateBuffer = m_renderGraph.CreateUAV(stateBuffer);
		passParameters->ValueCount = numValues;

		auto shader = GlobalShaderMap::Get<PrefixSumCS>();
		ComputeShaderUtils::AddPass<PrefixSumCS>(m_renderGraph,
			"PrefixSum",
			shader,
			passParameters,
			{ numGroups, 1, 1 });
	}
}
