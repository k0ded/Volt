#pragma once

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>

#include <glm/glm.hpp>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;

	struct RenderView;

	class GTAOTechnique
	{
	public:
		struct GTAOConstants
		{
			glm::ivec2 ViewportSize;
			glm::vec2 ViewportPixelSize;
			glm::vec2 DepthUnpackConsts;
			glm::vec2 CameraTanHalfFOV;
			glm::vec2 NDCToViewMul;
			glm::vec2 NDCToViewAdd;
			glm::vec2 NDCToViewMul_x_PixelSize;
			float EffectRadius;
			float EffectFalloffRange;
			float RadiusMultiplier;
			float Padding0;
			float FinalValuePower;
			float DenoiseBlurBeta;
			float SampleDistributionPower;
			float ThinOccluderCompensation;
			float DepthMIPSamplingOffset;
			int NoiseIndex;
		};

		GTAOTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		void Execute(const RenderView& view);

	private:
		struct MainPassOutput
		{
			RGTextureRef aoTerm;
			RGTextureRef edges;
		};

		RGTextureRef AddPrefilterDepthPass(const RenderView& view, RGUniformBufferRef gtaoConstants);
		MainPassOutput AddMainPass(const RenderView& view, RGUniformBufferRef gtaoConstants, RGTextureRef prefilteredDepth);
		void AddDenoisePass(const RenderView& view, RGUniformBufferRef gtaoConstants, RGTextureRef prefilteredDepth, const MainPassOutput& mainPassOutput);
		RGUniformBufferRef CreateUniformBuffer(const RenderView& view);

		RenderGraph& m_renderGraph;
		RenderGraphBlackboard& m_blackboard;
	};
}
