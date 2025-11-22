#pragma once

#include "Volt-Renderer/Debug/DebugVertices.h"

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>

#include <CoreUtilities/Allocators/FixedSizeLinearAllocator.h>

namespace Volt
{
	class RenderGraph;
	struct RenderView;

	class DebugRenderer
	{
	public:
		DebugRenderer();

		void DrawLine(const glm::vec3& v0, const glm::vec3& v1, const glm::vec4& color);
		void DrawLineSphere(const glm::vec3& center, float radius, const glm::vec4& color);

		void Render(RenderGraph& renderGraph, const RenderView& renderView, RGTextureRef dstTexture, RGTextureRef depthTexture);
		void Reset();

	private:
		void IniitalizeLineSphere();

		void RenderDebugLines(RenderGraph& renderGraph, const RenderView& renderView, RGTextureRef dstTexture, RGTextureRef depthTexture);

		struct LineSphere
		{
			Vector<glm::vec3> circleXZ;
			Vector<glm::vec3> circleXY;
			Vector<glm::vec3> circleYZ;

		} m_lineSphere;

		FixedSizeLinearAllocator<> m_lineVerticesAllocator;
	};
}
