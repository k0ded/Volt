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
		struct Result
		{
			RGTextureRef shadowMap;
			RGUniformBufferRef uniformBuffer;
		};

		CascadedDirectionalShadowTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		Result Execute(const RenderView& view, const RenderLightData& renderLightData);

	private:
		RGUniformBufferRef UploadUniformBufferData(const RenderView& view, const RenderLightData& renderLightData);

		RenderGraph& m_renderGraph;
		RenderGraphBlackboard& m_blackboard;
	};
}
