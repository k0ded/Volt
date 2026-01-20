#include "vrpch.h"

#include "Volt-Renderer/RenderingTechniques/BloomTechnique.h"
#include "Volt-Renderer/RenderView.h"
#include "Volt-Renderer/SceneRendererRenderGraphData.h"

#include <RenderCore/RenderGraph/ShaderRegistry.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderContext.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/PermutationCollection.h>

namespace Volt
{
	struct BloomDownsampleCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(BloomDownsampleCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, Source)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float3>, RWTarget)
			SHADER_PARAMETER(uint2, SrcResolution)
			SHADER_PARAMETER(uint2, TargetResolution)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(BloomDownsampleCS, "Engine/Shaders/Source/PostProcessing/Bloom/BloomDownsampleCS.hlsl", "BloomDownsampleCS", Compute);

	struct BloomUpsampleCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(BloomDownsampleCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float3>, Source)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float3>, RWTarget)
			SHADER_PARAMETER(uint2, TargetResolution)
			SHADER_PARAMETER(float, FilterRadius)
			SHADER_PARAMETER(float, BloomStrength)
		END_SHADER_PARAMETER_STRUCT()

		struct BloomComposite : SHADER_PERMUTATION_BOOL("BLOOM_COMPOSITE");
		using PermutationVector = PermutationCollection<BloomComposite>;
	};
	VT_REGISTER_SHADER(BloomUpsampleCS, "Engine/Shaders/Source/PostProcessing/Bloom/BloomUpsampleCS.hlsl", "BloomUpsampleCS", Compute);

	BloomTechnique::BloomTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
		: m_renderGraph(renderGraph),
		m_blackboard(blackboard)
	{}

	void BloomTechnique::Execute(const RenderView & view)
	{
		m_renderGraph.BeginMarker("Bloom");
		
		RGTextureRef intermediateTexture = AddDownsamplePass(view);
		AddUpsamplePass(view, intermediateTexture);

		m_renderGraph.EndMarker();
	}

	RGTextureRef BloomTechnique::AddDownsamplePass(const RenderView& view)
	{
		m_renderGraph.BeginMarker("Downsample");

		const SceneTextures& sceneTextures = m_blackboard.Get<SceneTextures>();

		const uint32_t width = static_cast<uint32_t>(glm::floor(view.width / 2.f));
		const uint32_t height = static_cast<uint32_t>(glm::floor(view.height / 2.f));

		RGTextureDesc textureDesc;
		textureDesc.width = width;
		textureDesc.height = height;
		textureDesc.debugName = "Bloom.Intermediate";
		textureDesc.mips = BloomTechnique::NumMips;
		textureDesc.format = RHI::PixelFormat::B10G11R11_UFLOAT_PACK32;
		textureDesc.usage = RHI::ImageUsage::Storage;

		RGTextureRef intermediateTexture = m_renderGraph.CreateTexture(textureDesc);
		RGTextureRef sourceTexture = sceneTextures.sceneColor;

		glm::uvec2 sourceResolution = { sourceTexture->GetDesc().width, sourceTexture->GetDesc().height };
		glm::uvec2 targetResolution = { width, height };

		auto shader = ShaderMap::Get<BloomDownsampleCS>();

		for (uint32_t i = 0; i < BloomTechnique::NumMips; ++i)
		{
			const uint32_t srcMip = i == 0 ? 0 : i - 1;

			BloomDownsampleCS::Parameters* passParameters = m_renderGraph.AllocParameters<BloomDownsampleCS::Parameters>();
			passParameters->RWTarget = m_renderGraph.CreateUAV(RGTextureUAVDesc{ .textureResource = intermediateTexture, .baseMipLevel = i, .mipCount = 1 });
			passParameters->Source = m_renderGraph.CreateSRV(RGTextureSRVDesc{ .textureResource = sourceTexture, .baseMipLevel = srcMip, .mipCount = 1 });
			passParameters->SrcResolution = sourceResolution;
			passParameters->TargetResolution = targetResolution;

			ComputeShaderUtils::AddPass<BloomDownsampleCS>(m_renderGraph,
				"BloomDownsampleCS",
				shader,
				passParameters,
				{ Math::DivideRoundUp(targetResolution.x, 8u), Math::DivideRoundUp(targetResolution.y, 8u), 1u });

			if (i == 0)
			{
				sourceTexture = intermediateTexture;
			}

			sourceResolution /= 2u;
			targetResolution /= 2u;
		}

		m_renderGraph.EndMarker();

		return intermediateTexture;
	}

	void BloomTechnique::AddUpsamplePass(const RenderView& view, RGTextureRef intermediateTexture)
	{
		m_renderGraph.BeginMarker("Upsample");

		const SceneTextures& sceneTextures = m_blackboard.Get<SceneTextures>();

		for (int32_t i = NumMips - 1; i >= 0; --i)
		{
			uint32_t targetWidth = view.width;
			uint32_t targetHeight = view.height;

			if (i != 0)
			{
				targetWidth = glm::max(1u, intermediateTexture->GetDesc().width >> (i - 1));
				targetHeight = glm::max(1u, intermediateTexture->GetDesc().height >> (i - 1));
			}

			BloomUpsampleCS::Parameters* passParameters = m_renderGraph.AllocParameters<BloomUpsampleCS::Parameters>();

			if (i != 0)
			{
				passParameters->RWTarget = m_renderGraph.CreateUAV(RGTextureUAVDesc{ .textureResource = intermediateTexture, .baseMipLevel = static_cast<uint32_t>(i) - 1, .mipCount = 1 });
			}
			else
			{
				passParameters->RWTarget = m_renderGraph.CreateUAV(sceneTextures.sceneColor);
			}

			passParameters->Source = m_renderGraph.CreateSRV(RGTextureSRVDesc{ .textureResource = intermediateTexture, .baseMipLevel = static_cast<uint32_t>(i), .mipCount = 1 });
			passParameters->TargetResolution = { targetWidth, targetHeight };
			passParameters->FilterRadius = 0.005f;
			passParameters->BloomStrength = 1.f;

			BloomUpsampleCS::PermutationVector permutationVector;
			permutationVector.Set<BloomUpsampleCS::BloomComposite>(i == 0);

			auto shader = ShaderMap::Get<BloomUpsampleCS>(permutationVector);

			ComputeShaderUtils::AddPass<BloomUpsampleCS>(m_renderGraph,
				"BloomUpsampleCS",
				shader,
				passParameters,
				{ Math::DivideRoundUp(targetWidth, 8u), Math::DivideRoundUp(targetHeight, 8u), 1u });
		}

		m_renderGraph.EndMarker();
	}
}
