#pragma once

#include <RenderCore/RenderGraph/Resources/RenderGraphResourceHandle.h>
#include <RHIModule/Images/Image.h>

#include <glm/glm.hpp>

namespace Volt
{
	namespace RHI
	{
		class Image2D;
	}

	class RenderGraph;
	class RenderGraphBlackboard;

	struct TAAData
	{
		RenderGraphImageHandle taaOutput;
		RenderGraphImageHandle accumulationOutput;
		RenderGraphImageHandle previousColor;
	};

	struct TAANoise
	{
		TAANoise();

		glm::vec2 Get(uint32_t frameIndex, const glm::uvec2& renderSize);

		inline static float s_haltonX[8];
		inline static float s_haltonY[8];
		inline static bool s_initialized = false;
	};

	class TAATechnique
	{
	public:
		TAATechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		TAAData Execute(RefPtr<RHI::Image> previousColor, RenderGraphImageHandle velocityTexture);

	private:
		RenderGraph& m_renderGraph;
		RenderGraphBlackboard& m_blackboard;
	};
}
