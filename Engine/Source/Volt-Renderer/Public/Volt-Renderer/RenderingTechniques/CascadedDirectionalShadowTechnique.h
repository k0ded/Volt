#pragma once

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;

	struct RenderView;
	struct RenderLightData;

	class CascadedDirectionalShadowTechnique
	{
	public:
		CascadedDirectionalShadowTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		void Execute(const RenderView& view, const RenderLightData& renderLightData);

	private:
		RGUniformBufferRef UploadUniformBufferData(const RenderView& view, const RenderLightData& renderLightData);

		RenderGraph& m_renderGraph;
		RenderGraphBlackboard& m_blackboard;
	};
}
