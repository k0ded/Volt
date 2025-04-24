#pragma once

#include "Volt-Renderer/SceneRendererStructs.h"

#include <RenderCore/RenderGraph/Resources/RenderGraphResourceHandle.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;

	class Camera;
	struct GTAOSettings;

	class GTAOTechnique
	{
	public:
		BEGIN_SHADER_PARAMETER_STRUCT(GTAOConstants)
			SHADER_PARAMETER(int2, ViewportSize)
			SHADER_PARAMETER(float2, ViewportPixelSize)
			SHADER_PARAMETER(float2, DepthUnpackConsts)
			SHADER_PARAMETER(float2, CameraTanHalfFOV)
			SHADER_PARAMETER(float2, NDCToViewMul)
			SHADER_PARAMETER(float2, NDCToViewAdd)
			SHADER_PARAMETER(float2, NDCToViewMul_x_PixelSize)
			SHADER_PARAMETER(float, EffectRadius)
			SHADER_PARAMETER(float, EffectFalloffRange)
			SHADER_PARAMETER(float, RadiusMultiplier)
			SHADER_PARAMETER(float, Padding0)
			SHADER_PARAMETER(float, FinalValuePower)
			SHADER_PARAMETER(float, DenoiseBlurBeta)
			SHADER_PARAMETER(float, SampleDistributionPower)
			SHADER_PARAMETER(float, ThinOccluderCompensation)
			SHADER_PARAMETER(float, DepthMIPSamplingOffset)
			SHADER_PARAMETER(int, NoiseIndex)
		END_SHADER_PARAMETER_STRUCT()

		GTAOTechnique(uint64_t frameIndex, const GTAOSettings& settings);
		GTAOOutput Execute(RenderGraph& frameGraph, RenderGraphBlackboard& blackboard, Ref<Camera> camera);

	private:
		friend struct PrefilterDepthData;

		void AddPrefilterDepthPass(RenderGraph& frameGraph, RenderGraphBlackboard& blackboard, Ref<Camera> camera);
		void AddMainPass(RenderGraph& frameGraph, RenderGraphBlackboard& blackboard);
		GTAOOutput AddDenoisePass(RenderGraph& frameGraph, RenderGraphBlackboard& blackboard);

		GTAOConstants m_constants;
		uint64_t m_frameIndex = 0;
	};
}
