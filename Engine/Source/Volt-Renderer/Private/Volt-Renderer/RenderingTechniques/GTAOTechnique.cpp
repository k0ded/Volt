#include "vrpch.h"
#include "Volt-Renderer/RenderingTechniques/GTAOTechnique.h"
#include "Volt-Renderer/Camera/Camera.h"
#include "Volt-Renderer/SceneRendererRenderGraphData.h"
#include "Volt-Renderer/RenderView.h"

#include <RenderCore/RenderGraph/ShaderRegistry.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderContext.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/SamplerStateCache.h>

#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	struct GTAODepthPrefilterCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(GTAODepthPrefilterCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float>, RWDepthMIP0)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float>, RWDepthMIP1)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float>, RWDepthMIP2)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float>, RWDepthMIP3)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float>, RWDepthMIP4)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float>, SourceDepth)
			SHADER_PARAMETER_SAMPLER(PointClampSampler)
			SHADER_PARAMETER_UNIFORM_BUFFER(GTAOConstants, Constants)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(GTAODepthPrefilterCS, "Engine/Shaders/Source/PostProcessing/GTAO/GTAO_DepthPrefilter.hlsl", "MainCS", Compute);

	struct GTAOMainPassCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(GTAOMainPassCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<uint>, RWAOTerm)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float>, RWEdges)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float>, SrcDepth)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, GBufferNormal)
			SHADER_PARAMETER_SAMPLER(PointClampSampler)
			SHADER_PARAMETER_UNIFORM_BUFFER(GTAOConstants, Constants)
			SHADER_PARAMETER(float4x4, ViewMatrix)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(GTAOMainPassCS, "Engine/Shaders/Source/PostProcessing/GTAO/GTAO_MainPass.hlsl", "MainCS", Compute);

	struct GTAODenoiseCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(GTAODenoiseCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<uint>, RWFinalAOTerm)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<uint>, AOTerm)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float>, Edges)
			SHADER_PARAMETER_SAMPLER(PointClampSampler)
			SHADER_PARAMETER_UNIFORM_BUFFER(GTAOConstants, Constants)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(GTAODenoiseCS, "Engine/Shaders/Source/PostProcessing/GTAO/GTAO_Denoise.hlsl", "MainCS", Compute);

	GTAOTechnique::GTAOTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
		: m_renderGraph(renderGraph), m_blackboard(blackboard)
	{
	}

	void GTAOTechnique::Execute(const RenderView& view)
	{
		m_renderGraph.BeginMarker("GTAO");

		RGUniformBufferRef gtaoUniformBuffer = CreateUniformBuffer(view);

		RGTextureRef prefilteredDepth = AddPrefilterDepthPass(view, gtaoUniformBuffer);
		MainPassOutput mainPassOutput = AddMainPass(view, gtaoUniformBuffer, prefilteredDepth);
		AddDenoisePass(view, gtaoUniformBuffer, prefilteredDepth, mainPassOutput);

		m_renderGraph.EndMarker();
	}

	RGTextureRef GTAOTechnique::AddPrefilterDepthPass(const RenderView& view, RGUniformBufferRef gtaoConstants)
	{
		constexpr uint32_t GTAO_PREFILTERED_DEPTH_MIP_COUNT = 5;

		const SceneTextures& sceneTextures = m_blackboard.Get<SceneTextures>();

		RGTextureDesc prefilteredDepthDesc = RGTextureDesc::Create2D<RHI::PixelFormat::R32_SFLOAT>(view.width, view.height, RHI::ImageUsage::Storage, "GTAO.PrefilteredDepth");
		prefilteredDepthDesc.mips = GTAO_PREFILTERED_DEPTH_MIP_COUNT;

		RGTextureRef prefilteredDepth = m_renderGraph.CreateTexture(prefilteredDepthDesc);

		GTAODepthPrefilterCS::Parameters* passParameters = m_renderGraph.AllocParameters<GTAODepthPrefilterCS::Parameters>();
		passParameters->RWDepthMIP0 = m_renderGraph.CreateUAV(RGTextureUAVDesc{ .textureResource = prefilteredDepth, .baseMipLevel = 0, .mipCount = 1 });
		passParameters->RWDepthMIP1 = m_renderGraph.CreateUAV(RGTextureUAVDesc{ .textureResource = prefilteredDepth, .baseMipLevel = 1, .mipCount = 1 });
		passParameters->RWDepthMIP2 = m_renderGraph.CreateUAV(RGTextureUAVDesc{ .textureResource = prefilteredDepth, .baseMipLevel = 2, .mipCount = 1 });
		passParameters->RWDepthMIP3 = m_renderGraph.CreateUAV(RGTextureUAVDesc{ .textureResource = prefilteredDepth, .baseMipLevel = 3, .mipCount = 1 });
		passParameters->RWDepthMIP4 = m_renderGraph.CreateUAV(RGTextureUAVDesc{ .textureResource = prefilteredDepth, .baseMipLevel = 4, .mipCount = 1 });
		passParameters->SourceDepth = m_renderGraph.CreateSRV(sceneTextures.sceneDepth);
		passParameters->PointClampSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureWrap::Clamp>();
		passParameters->Constants = gtaoConstants;

		const uint32_t dispatchX = Math::DivideRoundUp(view.width, 16u);
		const uint32_t dispatchY = Math::DivideRoundUp(view.height, 16u);

		auto shader = ShaderMap::Get<GTAODepthPrefilterCS>();
		ComputeShaderUtils::AddPass<GTAODepthPrefilterCS>(
			m_renderGraph,
			"GTAO.PrefilterDepth",
			shader,
			passParameters,
			{ dispatchX, dispatchY, 1 });

		return prefilteredDepth;
	}

	GTAOTechnique::MainPassOutput GTAOTechnique::AddMainPass(const RenderView& view, RGUniformBufferRef gtaoConstants, RGTextureRef prefilteredDepth)
	{
		const SceneTextures& sceneTextures = m_blackboard.Get<SceneTextures>();

		RGTextureRef aoOutput = m_renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R32_UINT>(view.width, view.height, RHI::ImageUsage::Storage, "GTAO.AO"));
		RGTextureRef edgesOutput = m_renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R8_UNORM>(view.width, view.height, RHI::ImageUsage::Storage, "GTAO.Edges"));

		GTAOMainPassCS::Parameters* passParameters = m_renderGraph.AllocParameters<GTAOMainPassCS::Parameters>();
		passParameters->RWAOTerm = m_renderGraph.CreateUAV(aoOutput);
		passParameters->RWEdges = m_renderGraph.CreateUAV(edgesOutput);
		passParameters->SrcDepth = m_renderGraph.CreateSRV(prefilteredDepth);
		passParameters->GBufferNormal = m_renderGraph.CreateSRV(sceneTextures.gBufferNormals);
		passParameters->ViewMatrix = view.camera->GetView();
		passParameters->PointClampSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureWrap::Clamp>();
		passParameters->Constants = gtaoConstants;

		const uint32_t dispatchX = Math::DivideRoundUp(view.width, 16u);
		const uint32_t dispatchY = Math::DivideRoundUp(view.height, 16u);

		auto shader = ShaderMap::Get<GTAOMainPassCS>();
		ComputeShaderUtils::AddPass<GTAOMainPassCS>(
			m_renderGraph,
			"GTAO.MainPass",
			shader,
			passParameters,
			{ dispatchX, dispatchY, 1 });

		return { aoOutput, edgesOutput };
	}

	void GTAOTechnique::AddDenoisePass(const RenderView& view, RGUniformBufferRef gtaoConstants, RGTextureRef prefilteredDepth, const MainPassOutput& mainPassOutput)
	{
		RGTextureRef finalAOTerm = m_renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R32_UINT>(view.width, view.height, RHI::ImageUsage::Storage, "GTAO.FinalAOTerm"));

		GTAODenoiseCS::Parameters* passParameters = m_renderGraph.AllocParameters<GTAODenoiseCS::Parameters>();
		passParameters->RWFinalAOTerm = m_renderGraph.CreateUAV(finalAOTerm);
		passParameters->AOTerm = m_renderGraph.CreateSRV(mainPassOutput.aoTerm);
		passParameters->Edges = m_renderGraph.CreateSRV(mainPassOutput.edges);
		passParameters->PointClampSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureWrap::Clamp>();
		passParameters->Constants = gtaoConstants;

		const uint32_t dispatchX = Math::DivideRoundUp(view.width, 8u);
		const uint32_t dispatchY = Math::DivideRoundUp(view.height, 8u);

		auto shader = ShaderMap::Get<GTAODenoiseCS>();
#if 0
		ComputeShaderUtils::AddPass<GTAODenoiseCS>(
			m_renderGraph,
			"GTAO.Denoise",
			shader,
			passParameters,
			{ dispatchX, dispatchY, 1 });
#else
		m_renderGraph.AddPass("GTAO.Denoise",
		RenderGraphPassFlags::Compute,
		passParameters,
		[passParameters, shader, dispatchX, dispatchY](RenderContext& context)
		{
			auto pipeline = PipelineStateCache::GetComputePipeline(shader);

			context.BindPipeline(pipeline);
			context.SetParameters<GTAODenoiseCS>(shader, passParameters);
			context.Dispatch(dispatchX, dispatchY, 1);
		});
#endif

		SceneTextures& sceneTextures = m_blackboard.Get<SceneTextures>();
		sceneTextures.sceneAO = finalAOTerm;
	}

	RGUniformBufferRef GTAOTechnique::CreateUniformBuffer(const RenderView& view)
	{
		const auto& projectionMatrix = view.camera->GetProjection();

		float depthLinearizeMul = (-projectionMatrix[3][2]);
		float depthLinearizeAdd = (projectionMatrix[2][2]);

		// correct the handedness issue
		if (depthLinearizeMul * depthLinearizeAdd < 0.f)
		{
			depthLinearizeAdd = -depthLinearizeAdd;
		}

		const float tanHalfFovY = 1.f / (projectionMatrix[1][1]);
		const float tanHalfFovX = 1.f / (projectionMatrix[0][0]);

		GTAOConstants gtaoConstants{};
		gtaoConstants.EffectRadius = 50.f;
		gtaoConstants.EffectFalloffRange = 0.615f;
		gtaoConstants.RadiusMultiplier = 1.457f;
		gtaoConstants.FinalValuePower = 2.2f;
		gtaoConstants.DenoiseBlurBeta = 1.2f;
		gtaoConstants.SampleDistributionPower = 2.f;
		gtaoConstants.ThinOccluderCompensation = 0.f;
		gtaoConstants.DepthMIPSamplingOffset = 3.3f;
		gtaoConstants.NoiseIndex = view.frameIndex % 64;
		gtaoConstants.ViewportSize = { view.width, view.height };
		gtaoConstants.ViewportPixelSize = { 1.f / static_cast<float>(view.width), 1.f / static_cast<float>(view.height) };
		gtaoConstants.DepthUnpackConsts = { depthLinearizeMul, depthLinearizeAdd };
		gtaoConstants.CameraTanHalfFOV = { tanHalfFovX, tanHalfFovY };
		gtaoConstants.NDCToViewMul = { gtaoConstants.CameraTanHalfFOV.x * 2.f, gtaoConstants.CameraTanHalfFOV.y * -2.f };
		gtaoConstants.NDCToViewAdd = { gtaoConstants.CameraTanHalfFOV.x * -1.f, gtaoConstants.CameraTanHalfFOV.y * 1.f };
		gtaoConstants.NDCToViewMul_x_PixelSize = { gtaoConstants.NDCToViewMul.x * gtaoConstants.ViewportPixelSize.x, gtaoConstants.NDCToViewMul.y * gtaoConstants.ViewportPixelSize.y };

		RGUniformBufferRef uniformBuffer = m_renderGraph.CreateUniformBuffer(RGUniformBufferDesc::Create<GTAOConstants>("GTAOConstants"));

		AddMappedBufferUpload(m_renderGraph, uniformBuffer, &gtaoConstants, sizeof(GTAOConstants));

		return uniformBuffer;
	}
}
