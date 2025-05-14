#pragma once

#include "Volt-Renderer/Config.h"

#include <RenderCore/RenderGraph/Resources/RenderGraphResourceHandle.h>

#include <glm/glm.hpp>

namespace Volt
{
	struct DrawCullingData
	{
		RenderGraphBufferHandle countCommandBuffer;
		RenderGraphBufferHandle taskCommandsBuffer;
		RenderGraphBufferHandle maskBuffer;
	};

	class RenderGraph;
	class RenderGraphBlackboard;

	class VTR_API CullingTechnique
	{
	public:
		enum class Type : uint32_t
		{
			Perspective = 0,
			Orthographic
		};

		struct Info
		{
			Type type = Type::Perspective;
			glm::mat4 viewMatrix;
			glm::vec4 cullingFrustum;
			float nearPlane;
			float farPlane;

			uint32_t drawCommandCount;
			uint32_t meshletCount;

			RenderGraphBufferHandle entityMaskBuffer = RenderGraphNullHandle();
		};

		CullingTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		DrawCullingData Execute(const Info& info);

	private:
		DrawCullingData AddDrawCallCullingPass(const Info& info);
		void AddTaskSubmitSetupPass(const DrawCullingData& data);

		RenderGraph& m_renderGraph;
		RenderGraphBlackboard& m_blackboard;
	};
}
