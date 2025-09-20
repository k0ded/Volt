#pragma once

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;

	struct RenderView;
	struct RenderLightData;

	class CascadedShadowMapsTechnique
	{
	public:
		struct Result
		{
			RGTextureRef shadowMap;
			RGUniformBufferRef uniformBuffer;
		};

		CascadedShadowMapsTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		Result Execute(const RenderView& view, const RenderLightData& renderLightData);

	private:
		RGUniformBufferRef UploadUniformBufferData(const RenderView& view, const RenderLightData& renderLightData);
		RGUniformBufferRef GenerateCascades(const RenderView& view, const RenderLightData& renderLightData);

		RenderGraph& m_renderGraph;
		RenderGraphBlackboard& m_blackboard;
	};
}
