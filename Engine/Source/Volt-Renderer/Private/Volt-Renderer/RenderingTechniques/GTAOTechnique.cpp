#include "vrpch.h"
#include "Volt-Renderer/RenderingTechniques/GTAOTechnique.h"

#include "Volt-Renderer/Renderer.h"
#include "Volt-Renderer/RendererCommon.h"
#include "Volt-Renderer/Camera/Camera.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	struct GTAODepthPrefilterCS
	{
		BEGIN_SHADER_DEFINITION(GTAODepthPrefilterCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/PostProcessing/GTAO/GTAO_DepthPrefilter_cs.hlsl", "main", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE_MIP(vt::RWTex2D<float>, RWDepthMIP0, 0)
			SHADER_PARAMETER_IMAGE_MIP(vt::RWTex2D<float>, RWDepthMIP1, 1)
			SHADER_PARAMETER_IMAGE_MIP(vt::RWTex2D<float>, RWDepthMIP2, 2)
			SHADER_PARAMETER_IMAGE_MIP(vt::RWTex2D<float>, RWDepthMIP3, 3)
			SHADER_PARAMETER_IMAGE_MIP(vt::RWTex2D<float>, RWDepthMIP4, 4)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float>, SourceDepth)
			SHADER_PARAMETER_SAMPLER(vt::TextureSampler, PointClampSampler)
			SHADER_PARAMETER_STRUCT(GTAOTechnique::GTAOConstants, Constants)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(GTAODepthPrefilterCS)

	struct GTAOMainPassCS
	{
		BEGIN_SHADER_DEFINITION(GTAOMainPassCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/PostProcessing/GTAO/GTAO_MainPass_cs.hlsl", "main", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::RWTex2D<uint>, AOTerm)
			SHADER_PARAMETER_IMAGE(vt::RWTex2D<float>, Edges)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float>, SrcDepth)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float4>, ViewspaceNormals)
			SHADER_PARAMETER_SAMPLER(vt::TextureSampler, PointClampSampler)
			SHADER_PARAMETER_STRUCT(GTAOTechnique::GTAOConstants, Constants)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(GTAOMainPassCS)

	struct GTAODenoiseCS
	{
		BEGIN_SHADER_DEFINITION(GTAODenoiseCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/PostProcessing/GTAO/GTAO_Denoise_cs.hlsl", "main", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::RWTex2D<uint>, RWFinalAOTerm)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<uint>, AOTerm)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float>, Edges)
			SHADER_PARAMETER_SAMPLER(vt::TextureSampler, PointClampSampler)
			SHADER_PARAMETER_STRUCT(GTAOTechnique::GTAOConstants, Constants)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(GTAODenoiseCS)

	struct PrefilterDepthData
	{
		RenderGraphImageHandle prefilteredDepth;
		GTAOTechnique::GTAOConstants constants{};
	};

	struct GTAOData
	{
		RenderGraphImageHandle aoOutput;
		RenderGraphImageHandle edgesOutput;
	};

	GTAOTechnique::GTAOTechnique(uint64_t frameIndex, const GTAOSettings& settings)
		: m_frameIndex(frameIndex)
	{
		// Setup constants
		{
			GTAOConstants constants{};

			///// Settings /////
			constants.EffectRadius = settings.radius;
			constants.EffectFalloffRange = settings.falloffRange;
			constants.DenoiseBlurBeta = 1.2f; // 1 denoise pass
			constants.RadiusMultiplier = settings.radiusMultiplier;
			constants.SampleDistributionPower = 2.f;
			constants.ThinOccluderCompensation = 0.f;
			constants.FinalValuePower = settings.finalValuePower;
			constants.DepthMIPSamplingOffset = 3.3f;
			constants.NoiseIndex = frameIndex % 64;
			constants.Padding0 = 0;

			m_constants = constants;
		}
	}

	GTAOOutput GTAOTechnique::Execute(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, Ref<Camera> camera)
	{
		renderGraph.BeginMarker("GTAO", { 0.f, 1.f, 0.f, 1.f });

		AddPrefilterDepthPass(renderGraph, blackboard, camera);
		AddMainPass(renderGraph, blackboard);
		GTAOOutput result = AddDenoisePass(renderGraph, blackboard);

		renderGraph.EndMarker();

		return result;
	}

	void GTAOTechnique::AddPrefilterDepthPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, Ref<Camera> camera)
	{
		constexpr uint32_t GTAO_PREFILTERED_DEPTH_MIP_COUNT = 5;

		const auto& renderData = blackboard.Get<ViewUniformBuffer>();

		m_constants.ViewportSize = renderData.renderSize;
		m_constants.ViewportPixelSize = { 1.f / static_cast<float>(renderData.renderSize.x), 1.f / static_cast<float>(renderData.renderSize.y) };

		const auto& projectionMatrix = camera->GetProjection();
		
		float depthLinearizeMul = (-projectionMatrix[3][2]);
		float depthLinearizeAdd = (projectionMatrix[2][2]);
		
		// correct the handedness issue
		if (depthLinearizeMul * depthLinearizeAdd < 0.f)
		{
			depthLinearizeAdd = -depthLinearizeAdd;
		}
		
		m_constants.DepthUnpackConsts = { depthLinearizeMul, depthLinearizeAdd };
		
		const float tanHalfFovY = 1.f / (projectionMatrix[1][1]);
		const float tanHalfFovX = 1.f / (projectionMatrix[0][0]);
		
		m_constants.CameraTanHalfFOV = { tanHalfFovX, tanHalfFovY };
		m_constants.NDCToViewMul = { m_constants.CameraTanHalfFOV.x * 2.f, m_constants.CameraTanHalfFOV.y * -2.f };
		m_constants.NDCToViewAdd = { m_constants.CameraTanHalfFOV.x * -1.f, m_constants.CameraTanHalfFOV.y * 1.f };
		m_constants.NDCToViewMul_x_PixelSize = { m_constants.NDCToViewMul.x * m_constants.ViewportPixelSize.x, m_constants.NDCToViewMul.y * m_constants.ViewportPixelSize.y };

		const auto& preDepthData = blackboard.Get<DepthPrePass>();

		blackboard.Add<PrefilterDepthData>() = renderGraph.AddPass<PrefilterDepthData>("GTAO Prefilter Depth Pass",
		[&](RenderGraph::Builder& builder, PrefilterDepthData& data) 
		{
			RenderGraphImageDesc desc{};
			desc.format = RHI::PixelFormat::R32_SFLOAT;
			desc.width = renderData.renderSize.x;
			desc.height = renderData.renderSize.y;
			desc.usage = RHI::ImageUsage::Storage;
			desc.mips = GTAO_PREFILTERED_DEPTH_MIP_COUNT;
			desc.name = "GTAO Prefiltered Depth";

			data.prefilteredDepth = builder.CreateImage(desc);
			data.constants = m_constants;

			builder.ReadResource(preDepthData.depth);
			builder.SetIsComputePass();
		},
		[=](const PrefilterDepthData& data, RenderContext& context) 
		{
			auto pipeline = ShaderMap::GetComputePipeline<GTAODepthPrefilterCS>();
			auto pointClampSampler = Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureWrap::Clamp>();

			GTAODepthPrefilterCS::Parameters parameters;
			parameters.RWDepthMIP0 = data.prefilteredDepth;
			parameters.RWDepthMIP1 = data.prefilteredDepth;
			parameters.RWDepthMIP2 = data.prefilteredDepth;
			parameters.RWDepthMIP3 = data.prefilteredDepth;
			parameters.RWDepthMIP4 = data.prefilteredDepth;
			parameters.SourceDepth = preDepthData.depth;
			parameters.PointClampSampler = pointClampSampler->GetResourceHandle();
			parameters.Constants = data.constants;

			context.BindPipeline(pipeline);
			context.SetParameters(parameters);

			const uint32_t dispatchX = Math::DivideRoundUp(renderData.renderSize.x, 16u);
			const uint32_t dispatchY = Math::DivideRoundUp(renderData.renderSize.y, 16u);
		
			context.Dispatch(dispatchX, dispatchY, 1);
		});
	}

	void GTAOTechnique::AddMainPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		const auto& prefilterDepthData = blackboard.Get<PrefilterDepthData>();
		const auto& preDepthData = blackboard.Get<DepthPrePass>();

		const glm::uvec2 renderSize = m_constants.ViewportSize;

		blackboard.Add<GTAOData>() = renderGraph.AddPass<GTAOData>("GTAO Main Pass",
		[&](RenderGraph::Builder& builder, GTAOData& data) 
		{
			// AO Texture
			{
				const auto desc = RGUtils::CreateImage2DDesc<RHI::PixelFormat::R32_UINT>(renderSize.x, renderSize.y, RHI::ImageUsage::Storage, "GTAO AO Output");
				data.aoOutput = builder.CreateImage(desc);
			}

			// Edges Texture
			{
				const auto desc = RGUtils::CreateImage2DDesc<RHI::PixelFormat::R8_UNORM>(renderSize.x, renderSize.y, RHI::ImageUsage::Storage, "GTAO Edges Output");
				data.edgesOutput = builder.CreateImage(desc);
			}

			builder.ReadResource(prefilterDepthData.prefilteredDepth);
			builder.ReadResource(preDepthData.normals);
			builder.SetIsComputePass();
		},
		[=](const GTAOData& data, RenderContext& context) 
		{
			auto pipeline = ShaderMap::GetComputePipeline<GTAOMainPassCS>();
			auto pointClampSampler = Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureWrap::Clamp>();

			GTAOMainPassCS::Parameters parameters;
			parameters.AOTerm = data.aoOutput;
			parameters.Edges = data.edgesOutput;
			parameters.SrcDepth = prefilterDepthData.prefilteredDepth;
			parameters.ViewspaceNormals = preDepthData.normals;
			parameters.PointClampSampler = pointClampSampler->GetResourceHandle();
			parameters.Constants = prefilterDepthData.constants;
			
			context.BindPipeline(pipeline);
			context.SetParameters(parameters);

			const uint32_t dispatchX = Math::DivideRoundUp(renderSize.x, 16u);
			const uint32_t dispatchY = Math::DivideRoundUp(renderSize.y, 16u);

			context.Dispatch(dispatchX, dispatchY, 1);
		});
	}

	GTAOOutput GTAOTechnique::AddDenoisePass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		const auto& gtaoData = blackboard.Get<GTAOData>();
		const auto& prefilterDepthData = blackboard.Get<PrefilterDepthData>();

		const glm::uvec2 renderSize = m_constants.ViewportSize;

		GTAOOutput& output = renderGraph.AddPass<GTAOOutput>("GTAO Denoise Pass 0",
		[&](RenderGraph::Builder& builder, GTAOOutput& data) 
		{
			{
				const auto desc = RGUtils::CreateImage2DDesc<RHI::PixelFormat::R32_UINT>(renderSize.x, renderSize.y, RHI::ImageUsage::Storage, "GTAO Final Output");
				data.outputImage = builder.CreateImage(desc);
			}

			{
				const auto desc = RGUtils::CreateImage2DDesc<RHI::PixelFormat::R32_UINT>(renderSize.x, renderSize.y, RHI::ImageUsage::Storage, "GTAO Temp Image");
				data.tempImage = builder.CreateImage(desc);
			}

			builder.ReadResource(gtaoData.aoOutput);
			builder.ReadResource(gtaoData.edgesOutput);

			builder.SetIsComputePass();
		},
		[=](const GTAOOutput& data, RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline<GTAODenoiseCS>();
			auto pointClampSampler = Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureWrap::Clamp>();
		
			GTAODenoiseCS::Parameters parameters;
			parameters.RWFinalAOTerm = data.outputImage;
			parameters.AOTerm = gtaoData.aoOutput;
			parameters.Edges = gtaoData.edgesOutput;
			parameters.PointClampSampler = pointClampSampler->GetResourceHandle();
			parameters.Constants = prefilterDepthData.constants;

			context.BindPipeline(pipeline);
			context.SetParameters(parameters);
		
			const uint32_t dispatchX = Math::DivideRoundUp(renderSize.x, 8u);
			const uint32_t dispatchY = Math::DivideRoundUp(renderSize.y, 8u);

			context.Dispatch(dispatchX, dispatchY, 1);
		});

		return output;
	}
}

