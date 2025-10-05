#pragma once

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>

namespace Volt
{
	namespace RHI
	{
		class Image;
	}

	class RenderGraph;
	class RenderGraphBlackboard;
	struct RenderView;

	class GlobalIlluminationRenderer
	{
	public:
		struct Output
		{
			RGTextureRef indirectLight;
		};

		Output Execute(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view);

	private:
		RefPtr<RHI::Image> m_prevIndirectLight;
	};
}
