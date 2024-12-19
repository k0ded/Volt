#include "vtpch.h"
#include "Volt/Rendering/RenderingTechniques/AutoExposureTechnique.h"

#include "Volt/Rendering/SceneRendererStructs.h"
#include "Volt/Rendering/RendererCommon.h"
#include "Volt/Math/Math.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/ShaderMap.h>

namespace Volt
{
	AutoExposureTechnique::AutoExposureTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
		: m_renderGraph(renderGraph), m_blackboard(blackboard)
	{}

	void AutoExposureTechnique::Execute(RenderGraphImageHandle srcRenderTarget, RenderGraphImageHandle averageLuminanceTarget, float deltaTime)
	{
		m_renderGraph.BeginMarker("Auto Exposure");

		RenderGraphBufferHandle histogramBuffer = GenerateLuminanceHistogram(srcRenderTarget);
		GenerateAverageLuminance(histogramBuffer, averageLuminanceTarget, deltaTime);
	
		m_renderGraph.EndMarker();
	}

	RenderGraphBufferHandle AutoExposureTechnique::GenerateLuminanceHistogram(RenderGraphImageHandle srcRenderTarget)
	{
		const auto& viewUniformBuffer = m_blackboard.Get<ViewUniformBuffer>();

		struct Data
		{
			RenderGraphBufferHandle histogramBuffer;
		};

		constexpr uint32_t HistogramBufferSize = 256;
		constexpr float MinLogLuminance = -10.f;
		constexpr float MaxLogLuminance = 2.f;
		constexpr float InvLogLuminanceRange = 1.f / (glm::abs(MinLogLuminance) + glm::abs(MaxLogLuminance));

		Data& outData = m_renderGraph.AddPass<Data>("Generate Luminance Histogram",
		[&](RenderGraph::Builder& builder, Data& data) 
		{
			data.histogramBuffer = builder.CreateBuffer(RGUtils::CreateBufferDescGPU<uint32_t>(HistogramBufferSize, "AutoExposure.HistogramBuffer"));
			
			builder.ReadResource(srcRenderTarget);

			builder.SetIsComputePass();
		},
		[=](const Data& data, RenderContext& context) 
		{
			auto pipeline = ShaderMap::GetComputePipeline("GenerateLuminanceHistogram");

			context.BindPipeline(pipeline);
			context.SetConstant("inputColor"_sh, srcRenderTarget);
			context.SetConstant("outHistogram"_sh, data.histogramBuffer);
			context.SetConstant("renderTargetSize"_sh, viewUniformBuffer.renderSize);
			context.SetConstant("minLogLum"_sh, MinLogLuminance);
			context.SetConstant("inverseLogLumRange"_sh, InvLogLuminanceRange);
		
			constexpr uint32_t ThreadGroupSize = 16;

			const uint32_t tileCountX = Math::DivideRoundUp(viewUniformBuffer.renderSize.x, ThreadGroupSize);
			const uint32_t tileCountY = Math::DivideRoundUp(viewUniformBuffer.renderSize.y, ThreadGroupSize);
		
			context.Dispatch(tileCountX, tileCountY, 1u);
		});

		return outData.histogramBuffer;
	}

	void AutoExposureTechnique::GenerateAverageLuminance(RenderGraphBufferHandle histogramBuffer, RenderGraphImageHandle averageLuminanceTarget, float deltaTime)
	{
		const auto& viewUniformBuffer = m_blackboard.Get<ViewUniformBuffer>();

		m_renderGraph.AddPass("Generate Average Luminance",
		[&](RenderGraph::Builder& builder) 
		{
			builder.WriteResource(histogramBuffer);
			builder.WriteResource(averageLuminanceTarget);

			builder.SetIsComputePass();
		},
		[=](RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline("GenerateAverageLuminance");

			context.BindPipeline(pipeline);
			context.SetConstant("outAverageLuminance"_sh, averageLuminanceTarget);
			context.SetConstant("histogramBuffer"_sh, histogramBuffer);
			context.SetConstant("totalPixelCount"_sh, viewUniformBuffer.renderSize.x * viewUniformBuffer.renderSize.y);
			context.SetConstant("logLumRange"_sh, 12.f);
			context.SetConstant("minLogLum"_sh, -10.f);
			context.SetConstant("blendFactor"_sh, 1.f - exp(-deltaTime * 1.1f));

			context.Dispatch(1, 1, 1);
		});
	}
}
