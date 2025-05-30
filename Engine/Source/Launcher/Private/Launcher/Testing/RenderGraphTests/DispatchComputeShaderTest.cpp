#include "Testing/RenderGraphTests/DispatchComputeShaderTest.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph2/RenderGraph2.h>
#include <RenderCore/RenderGraph2/RenderContext2.h>
#include <RenderCore/RenderGraph2/ShaderParameterStruct2.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/GlobalShader.h>

#include <CoreUtilities/Math/Math.h>

using namespace Volt;

struct DispatchComputeShaderTestCS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(DispatchComputeShaderTestCS)

	BEGIN_SHADER_PARAMETER_STRUCT2(Parameters)
		SHADER_PARAMETER_BUFFER_UAV(RWStructuredBuffer<uint>, OutputBuffer)
		SHADER_PARAMETER2(uint32_t, InitialValue)
	END_SHADER_PARAMETER_STRUCT2()
};
REGISTER_SHADER_2(DispatchComputeShaderTestCS, "Engine/Shaders/Source/Testing/RenderGraph/RG_DispatchComputeShaderTest.hlsl", "MainCS", Compute);

static RefPtr<RHI::Shader2> s_shader;
static RefPtr<RHI::ComputePipeline> s_pipeline;

RG_DispatchComputeShaderTest::RG_DispatchComputeShaderTest()
{
	s_shader = ShaderMap::Get2<DispatchComputeShaderTestCS>();
	s_pipeline = RHI::ComputePipeline::Create(s_shader);
}

RG_DispatchComputeShaderTest::~RG_DispatchComputeShaderTest()
{
}

bool RG_DispatchComputeShaderTest::RunTest()
{
	RenderGraph2 renderGraph{ m_commandBuffer };

	RGBufferRef dataBuffer = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDescGPU<glm::uvec2>(32));

	DispatchComputeShaderTestCS::Parameters* passParameters = renderGraph.AllocParameters<DispatchComputeShaderTestCS::Parameters>();
	passParameters->InitialValue = 1u;
	passParameters->OutputBuffer = renderGraph.CreateUAV(dataBuffer);
	 
	renderGraph.AddPass("Test", 
		RenderGraphPassFlags::Compute | RenderGraphPassFlags::NeverCull,
		passParameters, 
		[passParameters](RenderContext2& context) 
		{
			context.BindPipeline(s_pipeline);
			context.SetParameters<DispatchComputeShaderTestCS>(s_shader, passParameters);
			context.Dispatch(1, 1, 1);
		});

	renderGraph.Compile();
	renderGraph.Execute();

	//struct Data
	//{
	//	RenderGraphBufferHandle bufferHandle;
	//};
	//
	//DispatchComputeShaderTestCS::Parameters* passParameters = 
	//
	//renderGraph.AddPass<Data>("Compute Shader Pass",
	//[&](RenderGraph::Builder& builder, Data& data)
	//{
	//	{
	//		const auto desc = RGUtils::CreateBufferDescGPU<glm::uvec2>(32, "Buffer");
	//		data.bufferHandle = builder.CreateBuffer(desc);
	//	}
	//
	//	builder.SetHasSideEffect();
	//	builder.SetIsComputePass();
	//},
	//[=](const Data& data, RenderContext& context)
	//{
	//	auto pipeline = ShaderMap::GetComputePipeline<DispatchComputeShaderTestCS>();
	//
	//	context.BindPipeline(pipeline);
	//
	//	DispatchComputeShaderTestCS::Parameters parameters;
	//	parameters.InitialValue = 1u;
	//	parameters.OutputBuffer = data.bufferHandle;
	//
	//	context.SetParameters<DispatchComputeShaderTestCS>(parameters);
	//	context.Dispatch(1, 1, 1);
	//
	//});
	//
	//renderGraph.Compile();
	//renderGraph.Execute();

	return true;
}
