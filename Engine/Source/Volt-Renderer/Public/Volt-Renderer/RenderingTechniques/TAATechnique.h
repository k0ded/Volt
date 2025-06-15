#pragma once

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>

#include <RHIModule/Images/Image.h>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;
	struct RenderView;

	class TAATechnique
	{
	public:
		struct Output
		{
			RGTextureRef accumulation;
		};

		TAATechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		Output Execute(const RenderView& view, RefPtr<RHI::Image> prevAccumulation);

	private:
		RenderGraph& m_renderGraph;
		RenderGraphBlackboard& m_blackboard;
	};
}
