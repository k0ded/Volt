#include "vrpch.h"
#include "Volt-Renderer/RenderingTechniques/AutoExposureTechnique.h"

#include "Volt-Renderer/SceneRendererStructs.h"
#include "Volt-Renderer/RendererCommon.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	struct GenerateLuminanceHistogramCS
	{
		BEGIN_SHADER_DEFINITION(GenerateLuminanceHistogramCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Exposure/GenerateLuminanceHistogram.hlsl", "GenerateLuminanceHistogramCS", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float3>, InputColor)
			SHADER_PARAMETER_BUFFER(vt::RWTypedBuffer<uint>, RWHistogram)
			SHADER_PARAMETER(uint2, RenderTargetSize)
			SHADER_PARAMETER(float, MinLogLum)
			SHADER_PARAMETER(float, InverseLogLumRange)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(GenerateLuminanceHistogramCS)

	struct GenerateAverageLuminanceCS
	{
		BEGIN_SHADER_DEFINITION(GenerateAverageLuminanceCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Exposure/GenerateAverageLuminance.hlsl", "GenerateAverageLuminanceCS", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::RWTex2D<float>, RWAverageLuminance)
			SHADER_PARAMETER_BUFFER(vt::RWTypedBuffer<uint>, RWHistogramBuffer)
			SHADER_PARAMETER(uint, TotalPixelCount)
			SHADER_PARAMETER(float, LogLumRange)
			SHADER_PARAMETER(float, MinLogLum)
			SHADER_PARAMETER(float, BlendFactor)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(GenerateAverageLuminanceCS)

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
			auto pipeline = ShaderMap::GetComputePipeline<GenerateLuminanceHistogramCS>();

			GenerateLuminanceHistogramCS::Parameters parameters;
			parameters.InputColor = srcRenderTarget;
			parameters.RWHistogram = data.histogramBuffer;
			parameters.RenderTargetSize = viewUniformBuffer.renderSize;
			parameters.MinLogLum = MinLogLuminance;
			parameters.InverseLogLumRange = InvLogLuminanceRange;

			context.BindPipeline(pipeline);
			context.SetParameters(parameters);
		
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
			auto pipeline = ShaderMap::GetComputePipeline<GenerateAverageLuminanceCS>();

			GenerateAverageLuminanceCS::Parameters parameters;
			parameters.RWAverageLuminance = averageLuminanceTarget;
			parameters.RWHistogramBuffer = histogramBuffer;
			parameters.TotalPixelCount = viewUniformBuffer.renderSize.x * viewUniformBuffer.renderSize.y;
			parameters.LogLumRange = 12.f;
			parameters.MinLogLum = -10.f;
			parameters.BlendFactor = 1.f - exp(-deltaTime * 1.1f);

			context.BindPipeline(pipeline);
			context.SetParameters(parameters);
			context.Dispatch(1, 1, 1);
		});
	}
}
