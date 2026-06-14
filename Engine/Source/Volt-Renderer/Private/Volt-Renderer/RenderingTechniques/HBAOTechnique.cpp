#include "vrpch.h"
#include "Volt-Renderer/RenderingTechniques/HBAOTechnique.h"

#include <RenderCore/RenderGraph/ShaderRegistry.h>
#include <RenderCore/Shader/DefaultShaders.h>

#include "RenderView.h"
#include "SceneRendererRenderGraphData.h"
#include "RenderCore/SamplerStateCache.h"
#include "RenderCore/RenderGraph/RenderContext.h"
#include "RenderCore/RenderGraph/RenderGraph.h"
#include "RenderCore/RenderGraph/RenderGraphBlackboard.h"
#include "RenderCore/RenderGraph/ShaderParameterStruct.h"
#include "RenderCore/Shader/GlobalShader.h"
#include "RenderCore/Shader/GlobalShaderMap.h"
#include "RenderCore/Shader/PermutationCollection.h"
#include "Utility/ScatteredBufferUpload.h"

namespace Volt
{
	struct HBAOGenerateDataPS : Volt::GlobalShader
	{
		DECLARE_GLOBAL_SHADER(HBAOGenerateDataShader)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float>, NDCDepth)
			SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
			SHADER_PARAMETER_SAMPLER(PointClampSampler)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(HBAOGenerateDataPS, "Engine/Shaders/Source/PostProcessing/HBAO/GenerateData.hlsl", "MainPS", Pixel);

	// Splits workload into 16 distinct groups
	struct HBAODeinterleaveDepthCS : public Volt::GlobalShader
	{
		DECLARE_GLOBAL_SHADER(HBAODeinterleaveDepthCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER(float2, invFullSize)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float>, SrcTexture)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2DArray<float>, DstTexture)
			SHADER_PARAMETER_SAMPLER(PointClampSampler)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(HBAODeinterleaveDepthCS, "Engine/Shaders/Source/PostProcessing/HBAO/DeinterleaveDepth4x4.hlsl", "MainCS", Compute);


	struct HBAOMainPassGS : Volt::GlobalShader
	{
		DECLARE_GLOBAL_SHADER(HBAOMainPassGS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER(uint, arrayIndex)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(HBAOMainPassGS, "Engine/Shaders/Source/PostProcessing/HBAO/HBAOGS.hlsl", "MainGS", Geometry);

	// Runs 16 times
	struct HBAOMainPassPS : Volt::GlobalShader
	{
		DECLARE_GLOBAL_SHADER(HBAOMainPassPS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
			SHADER_PARAMETER_UNIFORM_BUFFER(HBAOConstants, constants)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2DArray<float>, HBAODeinterleavedDepth)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float3>, HBAOViewspaceNormals)
			SHADER_PARAMETER_SAMPLER(PointClampSampler)
			SHADER_PARAMETER(uint, arrayIndex)
			SHADER_PARAMETER(float2, rotation)
			SHADER_PARAMETER(float2, jitter)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(HBAOMainPassPS, "Engine/Shaders/Source/PostProcessing/HBAO/HBAO.hlsl", "MainPS", Pixel);

	// Recompiles 16 different images into one image.
	struct HBAOReinterleaveDataShader : Volt::GlobalShader
	{
		DECLARE_GLOBAL_SHADER(HBAOReinterleaveDataShader)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2DArray<float>, HBAODeinterleavedAO)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float2>, ReinterleavedAO)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(HBAOReinterleaveDataShader, "Engine/Shaders/Source/PostProcessing/HBAO/ReinterleaveData4x4.hlsl", "MainCS", Compute);

	// Separable blur on final image.
	struct HBAOBlurXCS : Volt::GlobalShader
	{
		DECLARE_GLOBAL_SHADER(HBAOBlurXCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(HBAOConstants, constants)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float2>, SrcTexture)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float2>, DstTexture)
			SHADER_PARAMETER_SAMPLER(PointClampSampler)
			SHADER_PARAMETER_SAMPLER(BilinearClampSampler)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(HBAOBlurXCS, "Engine/Shaders/Source/PostProcessing/HBAO/HBAOSeparableBlurX.hlsl", "MainCS", Compute);

	struct HBAOBlurYCS : Volt::GlobalShader
	{
		DECLARE_GLOBAL_SHADER(HBAOBlurYCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(HBAOConstants, constants)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float2>, SrcTexture)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float>, DstTexture)
			SHADER_PARAMETER_SAMPLER(PointClampSampler)
			SHADER_PARAMETER_SAMPLER(BilinearClampSampler)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(HBAOBlurYCS, "Engine/Shaders/Source/PostProcessing/HBAO/HBAOSeparableBlurY.hlsl", "MainCS", Compute);

	BEGIN_SHADER_PARAMETER_STRUCT(HBAOGenerateDataParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(FullscreenTriangleVS::Parameters, VS)
		SHADER_PARAMETER_STRUCT_INCLUDE(HBAOGenerateDataPS::Parameters, PS)
	END_SHADER_PARAMETER_STRUCT()

	BEGIN_SHADER_PARAMETER_STRUCT(HBAOMainPassParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(FullscreenTriangleVS::Parameters, VS)
		SHADER_PARAMETER_STRUCT_INCLUDE(HBAOMainPassGS::Parameters, GS)
		SHADER_PARAMETER_STRUCT_INCLUDE(HBAOMainPassPS::Parameters, PS)
	END_SHADER_PARAMETER_STRUCT()

	Volt::HBAOTechnique::HBAOTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
		: m_renderGraph(renderGraph), m_blackboard(blackboard)
	{
		static constexpr float randomNumbers[48]
		{
			0.463937f,0.340042f,0.223035f,0.468465f,0.322224f,0.979269f,0.031798f,0.973392f,0.778313f,0.456168f,0.258593f,0.330083f,0.387332f,0.380117f,0.179842f,0.910755f,
			0.511623f,0.092933f,0.180794f,0.620153f,0.101348f,0.556342f,0.642479f,0.442008f,0.215115f,0.475218f,0.157357f,0.568868f,0.501241f,0.629229f,0.699218f,0.707733f,
			0.556725f,0.005520f,0.708315f,0.583199f,0.236644f,0.992380f,0.981091f,0.119804f,0.510866f,0.560499f,0.961497f,0.557862f,0.539955f,0.332871f,0.417807f,0.920779f,
		};

		int r = 0;
		for (auto& i : Jitter)
		{
			float r1 = randomNumbers[r++];
			float r2 = randomNumbers[r++];
			float r3 = randomNumbers[r++];

			float angle = r1 * 3.14159f / 4.f;
			i = { cos(angle), sin(angle), r2, r3 };
		}
	}

	void Volt::HBAOTechnique::Execute(const RenderView& view)
	{
		m_renderGraph.BeginMarker("HBAO");

		RGUniformBufferRef hbaoBuffer = CreateUniformBuffer(view);
		GenerateDataOutput generateData = AddGenerateDataPass(view);
		RGTextureRef deinterleavedDepth = DeinterleaveDepth(view, generateData.linearDepth);
		RGTextureRef deinterleavedAO = AddHBAOPass(view, deinterleavedDepth, generateData.viewNormal, hbaoBuffer);
		RGTextureRef reinterleavedAO = ReinterleaveAO(view, deinterleavedAO);
		RGTextureRef finalAO = BlurHBAO(view, reinterleavedAO, hbaoBuffer);
		SceneTextures& sceneTextures = m_blackboard.Get<SceneTextures>();
		sceneTextures.sceneAO = finalAO;

		m_renderGraph.EndMarker();
	}

	HBAOTechnique::GenerateDataOutput HBAOTechnique::AddGenerateDataPass(const RenderView& view)
	{
		uint2 aoSize = GetAOSize(view.width, view.height);
		RGTextureDesc linearDepthDesc = RGTextureDesc::Create2D<RHI::PixelFormat::R32_SFLOAT>(aoSize.x, aoSize.y, RHI::ImageUsage::Attachment, "HBAO.LinearDepth");
		RGTextureRef linearDepth = m_renderGraph.CreateTexture(linearDepthDesc);

		RGTextureDesc viewNormalDesc = RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_UNORM>(aoSize.x, aoSize.y, RHI::ImageUsage::Attachment, "HBAO.ViewNormals");
		RGTextureRef viewNormal = m_renderGraph.CreateTexture(viewNormalDesc);

		const SceneTextures& sceneTextures = m_blackboard.Get<SceneTextures>();

		HBAOGenerateDataParameters* passParameters = m_renderGraph.AllocParameters<HBAOGenerateDataParameters>();
		passParameters->PS.NDCDepth = m_renderGraph.CreateSRV(sceneTextures.sceneDepth);
		passParameters->PS.View = view.viewUniformBuffer;
		passParameters->PS.PointClampSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureWrap::Clamp>();
		passParameters->PS.renderTargets.renderTargets[0] = linearDepth;
		passParameters->PS.renderTargets.renderTargets[1] = viewNormal;

		auto vs = GlobalShaderMap::Get<FullscreenTriangleVS>();
		auto ps = GlobalShaderMap::Get<HBAOGenerateDataPS>();
		m_renderGraph.AddPass(
			"HBAO.GenerateData",
			RenderGraphPassFlags::Raster,
			passParameters,
			[passParameters, aoSize, vs, ps](RenderContext& context)
			{
				RenderingInfo info = context.CreateRenderingInfo(aoSize.x, aoSize.y, passParameters->PS.renderTargets);
				GraphicsPipelineState state{};
				state.shaders.emplace_back(vs);
				state.shaders.emplace_back(ps);
				state.depthMode = RHI::DepthMode::None;
				state.cullMode = RHI::CullMode::None;
				state.renderTargets = passParameters->PS.renderTargets;
				context.BeginRendering(info);
				context.SetPipelineState(state);
				context.SetParameters<HBAOGenerateDataPS>(ps, &passParameters->PS);
				context.Draw(3, 1, 0, 0);
				context.EndRendering();
			}
		);

		return GenerateDataOutput(linearDepth, viewNormal);
	}

	RGTextureRef HBAOTechnique::DeinterleaveDepth(const RenderView& view, RGTextureRef linearDepth)
	{
		uint2 aoSize = GetAOSize(view.width, view.height);
		uint2 quarterRes = { Math::DivideRoundUp(aoSize.x, 4u), Math::DivideRoundUp(aoSize.y, 4u) };
		RGTextureDesc linearDepthDesc = RGTextureDesc::Create2D<RHI::PixelFormat::R32_SFLOAT>(
			quarterRes.x,
			quarterRes.y,
			RHI::ImageUsage::Storage, 
			"HBAO.DeinterleavedDepth"
		);
		linearDepthDesc.layers = 16;
		RGTextureRef linearDepthArray = m_renderGraph.CreateTexture(linearDepthDesc);

		HBAODeinterleaveDepthCS::Parameters* passParameters = m_renderGraph.AllocParameters<HBAODeinterleaveDepthCS::Parameters>();
		passParameters->SrcTexture = m_renderGraph.CreateSRV(linearDepth);
		passParameters->DstTexture = m_renderGraph.CreateUAV(linearDepthArray);
		passParameters->invFullSize = 1.0f / float2(aoSize.x, aoSize.y);
		passParameters->PointClampSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureWrap::Clamp>();

		auto cs = GlobalShaderMap::Get<HBAODeinterleaveDepthCS>();
		ComputeShaderUtils::AddPass<HBAODeinterleaveDepthCS>(
			m_renderGraph,
			"HBAO.DeinterleaveDepth",
			cs,
			passParameters,
			{ Math::DivideRoundUp(quarterRes.x, 4u), Math::DivideRoundUp(quarterRes.y, 4u), 1u }
		);

		return linearDepthArray;
	}

	RGTextureRef HBAOTechnique::AddHBAOPass(const RenderView& view, RGTextureRef deinterleavedDepth, RGTextureRef viewNormals, RGUniformBufferRef hbaoConstants)
	{
		uint2 aoSize = GetAOSize(view.width, view.height);
		uint2 quarterRes = { Math::DivideRoundUp(aoSize.x, 4u), Math::DivideRoundUp(aoSize.y, 4u) };
		RGTextureDesc aoDesc = RGTextureDesc::Create2D<RHI::PixelFormat::R16G16_SFLOAT>(
			quarterRes.x,
			quarterRes.y,
			RHI::ImageUsage::Attachment,
			"HBAO.DeinterleavedAO"
		);
		aoDesc.initializeImage = true;
		aoDesc.layers = 16;
		RGTextureRef deinterleavedAO = m_renderGraph.CreateTexture(aoDesc);

		auto vs = GlobalShaderMap::Get<FullscreenTriangleVS>();
		auto gs = GlobalShaderMap::Get<HBAOMainPassGS>();
		auto ps = GlobalShaderMap::Get<HBAOMainPassPS>();


		struct InlineParameters
		{
			float2 jitter;
			float2 rotation;
			uint arrayIndex;
		};
		bool useInlineParameters = true;

		if (useInlineParameters)
		{
			float4* jitterData = (float4*)m_renderGraph.AllocData(sizeof(Jitter));
			memcpy(jitterData, Jitter, sizeof(Jitter));

			HBAOMainPassParameters* passParameters = m_renderGraph.AllocParameters<HBAOMainPassParameters>();
			passParameters->PS.View = view.viewUniformBuffer;
			passParameters->PS.constants = hbaoConstants;
			passParameters->PS.HBAODeinterleavedDepth = m_renderGraph.CreateSRV(deinterleavedDepth);
			passParameters->PS.HBAOViewspaceNormals = m_renderGraph.CreateSRV(viewNormals);
			passParameters->PS.renderTargets.renderTargets[0] = deinterleavedAO;
			passParameters->PS.PointClampSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureWrap::Clamp>();
			
			passParameters->PS.rotation = { Jitter[0].x, Jitter[0].y };
			passParameters->PS.jitter = { Jitter[0].z, Jitter[0].w };
			passParameters->PS.arrayIndex = 0;
			passParameters->GS.arrayIndex = 0;
			
			m_renderGraph.AddPass(
			"HBAO.MainPass",
			RenderGraphPassFlags::Raster,
			passParameters,
			[jitterData, vs, gs, ps, passParameters, quarterRes](RenderContext& context)
			{
				RenderingInfo info = context.CreateRenderingInfo(quarterRes.x, quarterRes.y, passParameters->PS.renderTargets);
				//info.renderingInfo.colorAttachments[0].clearMode = RHI::ClearMode::DontCare;
				info.renderingInfo.layerCount = 16;
				GraphicsPipelineState state = {};
				state.shaders.push_back(vs);
				state.shaders.push_back(gs);
				state.shaders.push_back(ps);
				state.depthMode = RHI::DepthMode::None;
				state.cullMode = RHI::CullMode::None;
				state.renderTargets = passParameters->PS.renderTargets;
				context.BeginRendering(info);
				context.SetPipelineState(state);
				context.SetParameters<HBAOMainPassGS>(gs, &passParameters->GS);
				context.SetParameters<HBAOMainPassPS>(ps, &passParameters->PS);

				
				for (int i = 0; i < 16; ++i)
				{
					InlineParameters parameters;
					parameters.rotation = { jitterData[i].x, jitterData[i].y };
					parameters.jitter = { jitterData[i].z, jitterData[i].w };
					parameters.arrayIndex = i;
					context.GetRHICommandBuffer()->PushInlineParameters(&parameters, sizeof(InlineParameters), 0, RHI::ShaderStage::Geometry | RHI::ShaderStage::Pixel);
					context.Draw(3, 1, 0, 0);
				}
				context.EndRendering();
			}
			);
		}
		else
		{
			for (int i = 0; i < 16; ++i)
			{
				HBAOMainPassParameters* passParameters = m_renderGraph.AllocParameters<HBAOMainPassParameters>();
				passParameters->PS.View = view.viewUniformBuffer;
				passParameters->PS.constants = hbaoConstants;
				passParameters->PS.HBAODeinterleavedDepth = m_renderGraph.CreateSRV(deinterleavedDepth);
				passParameters->PS.HBAOViewspaceNormals = m_renderGraph.CreateSRV(viewNormals);
				passParameters->PS.renderTargets.renderTargets[0] = deinterleavedAO;
				passParameters->PS.PointClampSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureWrap::Clamp>();

				passParameters->PS.rotation = { Jitter[i].x, Jitter[i].y };
				passParameters->PS.jitter = { Jitter[i].z, Jitter[i].w };
				passParameters->PS.arrayIndex = i;
				passParameters->GS.arrayIndex = i;

				m_renderGraph.AddPass(
				std::format("HBAO.MainPass {}", i).c_str(),
				RenderGraphPassFlags::Raster,
				passParameters,
				[vs, gs, ps, passParameters, quarterRes](RenderContext& context)
				{
					RenderingInfo info = context.CreateRenderingInfo(quarterRes.x, quarterRes.y, passParameters->PS.renderTargets);
					info.renderingInfo.colorAttachments[0].clearMode = RHI::ClearMode::DontCare;
					info.renderingInfo.layerCount = 16;
					GraphicsPipelineState state = {};
					state.shaders.push_back(vs);
					state.shaders.push_back(gs);
					state.shaders.push_back(ps);
					state.depthMode = RHI::DepthMode::None;
					state.cullMode = RHI::CullMode::None;
					state.renderTargets = passParameters->PS.renderTargets;
					context.BeginRendering(info);
					context.SetPipelineState(state);
					context.SetParameters<HBAOMainPassGS>(gs, &passParameters->GS);
					context.SetParameters<HBAOMainPassPS>(ps, &passParameters->PS);
					context.Draw(3, 1, 0, 0);
					context.EndRendering();
				}
				);
			}
		}

		return deinterleavedAO;
	}

	RGTextureRef HBAOTechnique::ReinterleaveAO(const RenderView& view, RGTextureRef deinterleavedAO)
	{
		uint2 aoSize = GetAOSize(view.width, view.height);
		RGTextureDesc aoDesc = RGTextureDesc::Create2D<RHI::PixelFormat::R16G16_SFLOAT>(aoSize.x, aoSize.y, RHI::ImageUsage::Storage, "HBAO.ReinterleavedAO");
		RGTextureRef reinterleavedAOZ = m_renderGraph.CreateTexture(aoDesc);

		HBAOReinterleaveDataShader::Parameters* passParameters = m_renderGraph.AllocParameters<HBAOReinterleaveDataShader::Parameters>();
		passParameters->HBAODeinterleavedAO = m_renderGraph.CreateSRV(deinterleavedAO);
		passParameters->ReinterleavedAO = m_renderGraph.CreateUAV(reinterleavedAOZ);

		auto cs = GlobalShaderMap::Get<HBAOReinterleaveDataShader>();
		ComputeShaderUtils::AddPass<HBAOReinterleaveDataShader>(
			m_renderGraph,
			"HBAO.ReinterleaveAO",
			cs,
			passParameters,
			{ Math::DivideRoundUp(aoSize.x, 8u), Math::DivideRoundUp(aoSize.y, 8u), 1u }
		);

		return reinterleavedAOZ;
	}

	RGTextureRef HBAOTechnique::BlurHBAO(const RenderView& view, RGTextureRef reinterleavedAOZ, RGUniformBufferRef hbaoConstants)
	{
		uint2 aoSize = GetAOSize(view.width, view.height);
		RGTextureDesc aoDesc = RGTextureDesc::Create2D<RHI::PixelFormat::R16G16_SFLOAT>(aoSize.x, aoSize.y, RHI::ImageUsage::Storage, "HBAO.IntermediateAOZ");
		RGTextureRef intermediateAOZ = m_renderGraph.CreateTexture(aoDesc);

		HBAOBlurXCS::Parameters* xParams = m_renderGraph.AllocParameters<HBAOBlurXCS::Parameters>();
		xParams->constants = hbaoConstants;
		xParams->SrcTexture = m_renderGraph.CreateSRV(reinterleavedAOZ);
		xParams->DstTexture = m_renderGraph.CreateUAV(intermediateAOZ);
		xParams->PointClampSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureWrap::Clamp>();
		xParams->BilinearClampSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Clamp>();
			
		auto cs = GlobalShaderMap::Get<HBAOBlurXCS>();
		ComputeShaderUtils::AddPass<HBAOBlurXCS>(
			m_renderGraph,
			"HBAO.BlurX",
			cs,
			xParams,
			{ Math::DivideRoundUp(aoSize.x, 8u), Math::DivideRoundUp(aoSize.y, 8u), 1u }
		);

		RGTextureDesc finalAODesc = RGTextureDesc::Create2D<RHI::PixelFormat::R8_UNORM>(aoSize.x, aoSize.y, RHI::ImageUsage::Storage, "HBAO.FinalAO");
		RGTextureRef finalAO = m_renderGraph.CreateTexture(finalAODesc);

		HBAOBlurYCS::Parameters* yParams = m_renderGraph.AllocParameters<HBAOBlurYCS::Parameters>();
		yParams->constants = hbaoConstants;
		yParams->SrcTexture = m_renderGraph.CreateSRV(reinterleavedAOZ);
		yParams->DstTexture = m_renderGraph.CreateUAV(finalAO);
		yParams->PointClampSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureWrap::Clamp>();
		yParams->BilinearClampSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Clamp>();

		cs = GlobalShaderMap::Get<HBAOBlurYCS>();
		ComputeShaderUtils::AddPass<HBAOBlurYCS>(
			m_renderGraph,
			"HBAO.FinalizeAO",
			cs,
			yParams,
			{ Math::DivideRoundUp(aoSize.x, 8u), Math::DivideRoundUp(aoSize.y, 8u), 1u }
		);

		return finalAO;
	}

	uint2 HBAOTechnique::GetAOSize(uint aScreenWidth, uint aScreenHeight)
	{
		// Minimal IQ gains from sampling working on a resolution over 1080p but the time to finish the pass explodes, so lets just clamp the AO size to 1080p. 
		// Alternative could be running the pass at quarter res, but that would drop IQ significantly. Only tested on a 1440p monitor, might be different on a 4k monitor but since
		// AO is low frequency I doubt it :P
		return uint2(std::min(aScreenWidth, 1920u), std::min(aScreenHeight, 1080u));
	}

	RGUniformBufferRef HBAOTechnique::CreateUniformBuffer(const RenderView& view)
	{
		float2 fullscreenSize = float2(view.width, view.height);
		float2 aoSize = GetAOSize(view.width, view.height);

		float radius = 100; // 1-meter radius seems reasonable
		float pow = 4; // Reasonable default. Should be in the range [1, 5]. Artistic choice

		HBAOConstants hbaoConstants;
		hbaoConstants.AOSize = aoSize;
		hbaoConstants.InvAOSize = 1.0f / aoSize;
		hbaoConstants.InvFullSize = 1.0f / fullscreenSize;
		hbaoConstants.radius = radius;
		hbaoConstants.R2 = radius * radius;
		hbaoConstants.InvNegR2 = -1 / hbaoConstants.R2;
		hbaoConstants.power = pow;

		RGUniformBufferRef uniformBuffer = m_renderGraph.CreateUniformBuffer(RGUniformBufferDesc::Create<HBAOConstants>("HBAOConstants"));
		AddMappedBufferUploadCopyData(m_renderGraph, uniformBuffer, &hbaoConstants, sizeof(HBAOConstants));
		return uniformBuffer;
	}
}
