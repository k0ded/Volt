#pragma once

#include "Volt-Renderer/Config.h"

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>

namespace Volt
{
	class RenderGraph;

	class PrefixSumTechnique
	{
	public:
		VTR_API PrefixSumTechnique(RenderGraph& renderGraph);
		VTR_API void Execute(RGBufferRef inputBuffer, RGBufferRef outputBuffer, uint32_t numValues);

	private:
		RenderGraph& m_renderGraph;
	};
}
