#pragma once

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>

#include <glm/glm.hpp>

#include "RenderCore/RenderGraph/ShaderTypes.h"

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;

	struct RenderView;

	class HBAOTechnique
	{
	public:
		struct HBAOConstants
		{
			float2 AOSize;
			float2 InvAOSize;
			float2 InvFullSize;
			float R2;
			float radius;
			float InvNegR2;
			float power; // Around [1, 5] looks good. Default: 4
			float2 padding;
		};

		HBAOTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		void Execute(const RenderView& view);

	private:
		struct GenerateDataOutput
		{
			RGTextureRef linearDepth;
			RGTextureRef viewNormal;
		};
		float4 Jitter[16];

		GenerateDataOutput AddGenerateDataPass(const RenderView& view);

		RGTextureRef DeinterleaveDepth(const RenderView& view, RGTextureRef linearDepth);
		RGTextureRef AddHBAOPass(const RenderView& view, RGTextureRef deinterleavedDepth, RGTextureRef viewNormals, RGUniformBufferRef hbaoConstants);
		RGTextureRef ReinterleaveAO(const RenderView& view, RGTextureRef deinterleavedAO);
		RGTextureRef BlurHBAO(const RenderView& view, RGTextureRef reinterleavedAOZ, RGUniformBufferRef hbaoConstants);

		uint2 GetAOSize(uint aScreenWidth, uint aScreenHeight);

		RGUniformBufferRef CreateUniformBuffer(const RenderView& view);

		RenderGraph& m_renderGraph;
		RenderGraphBlackboard& m_blackboard;
	};
}
