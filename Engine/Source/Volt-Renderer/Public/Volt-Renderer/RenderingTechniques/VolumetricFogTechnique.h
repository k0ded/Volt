#pragma once

#include <RenderCore/RenderGraph/Resources/RenderGraphResourceHandle.h>

#include <RHIModule/Images/Image.h>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;

	struct VolumetricFogData
	{
		RenderGraphUniformBufferHandle fogParamsBuffer;
		RenderGraphImageHandle integratedFogVolume;
	};

	class VolumetricFogTechnique
	{
	public:
		VolumetricFogData Execute(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);

	private:
		RenderGraphImageHandle ExecuteInjectExtinctionScattering(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphUniformBufferHandle volumetricFogParamsBuffer);
		RenderGraphImageHandle ExecuteLightScattering(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphUniformBufferHandle volumetricFogParamsBuffer, RenderGraphImageHandle scatteringExtinctionImage);
		RenderGraphImageHandle ExecuteSpatialFilter(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphImageHandle lightScatteringImage);
		void ExecuteTemporalFilter(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphUniformBufferHandle volumetricFogParamsBuffer, RenderGraphImageHandle lightScatteringImage);
		RenderGraphImageHandle ExecuteIntegration(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphUniformBufferHandle volumetricFogParamsBuffer, RenderGraphImageHandle lightScatteringImage);

		RefPtr<RHI::Image> m_previousLightScatteringImage;
	};
}
