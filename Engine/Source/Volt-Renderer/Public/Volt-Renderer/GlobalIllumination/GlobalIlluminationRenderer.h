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
			RGTextureRef indirectLight = nullptr;
		};

		GlobalIlluminationRenderer();

		Output Execute(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view);

		void Visualize(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view);

	private:
		RGUniformBufferRef SetupIrradianceVolumeUniformBuffer(RenderGraph& renderGraph, const RenderView& view);

		RefPtr<RHI::StorageBuffer> m_spatialHashTableChecksumBuffer;
		RefPtr<RHI::StorageBuffer> m_worldRadianceCacheCellCache;
		
		RefPtr<RHI::Image> m_prevIndirectLight;
	};
}
