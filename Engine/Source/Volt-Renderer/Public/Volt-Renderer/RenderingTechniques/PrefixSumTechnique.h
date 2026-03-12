#pragma once

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>

namespace Volt
{
	class RenderGraph;

	class PrefixSumTechnique
	{
	public:
		PrefixSumTechnique(RenderGraph& renderGraph);
		void Execute(RGBufferRef inputBuffer, RGBufferRef outputBuffer, uint32_t numValues);

	private:
		RenderGraph& m_renderGraph;
	};
}
