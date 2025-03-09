#include "Testing/RenderGraphTests/DispatchComputeShaderTest.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/ShaderParameterStruct.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <CoreUtilities/Math/Math.h>

using namespace Volt;

BEGIN_SHADER_PARAMETER_STRUCT(DispatchComputeShaderParameters)
	SHADER_PARAMETER_BUFFER(vt::RWTypedBuffer<uint>, OutputBuffer)
	SHADER_PARAMETER(uint32_t, InitialValue)
END_SHADER_PARAMETER_STRUCT()

RG_DispatchComputeShaderTest::RG_DispatchComputeShaderTest()
{
}

RG_DispatchComputeShaderTest::~RG_DispatchComputeShaderTest()
{
}

bool RG_DispatchComputeShaderTest::RunTest()
{
	RenderGraph renderGraph{ m_commandBuffer };

	struct Data
	{
		RenderGraphBufferHandle bufferHandle;
	};

	renderGraph.AddPass<Data>("Compute Shader Pass",
	[&](RenderGraph::Builder& builder, Data& data)
	{
		{
			const auto desc = RGUtils::CreateBufferDescGPU<glm::uvec2>(32, "Buffer");
			data.bufferHandle = builder.CreateBuffer(desc);
		}

		builder.SetHasSideEffect();
		builder.SetIsComputePass();
	},
	[=](const Data& data, RenderContext& context)
	{
		auto pipeline = ShaderMap::GetComputePipeline("RG_DispatchComputeShaderTest");

		context.BindPipeline(pipeline);

		DispatchComputeShaderParameters parameters;
		parameters.InitialValue = 1u;
		parameters.OutputBuffer = data.bufferHandle;

		context.SetParameters(parameters);

		context.Dispatch(1, 1, 1);

	});

	renderGraph.Compile();
	renderGraph.Execute();

	return true;
}
