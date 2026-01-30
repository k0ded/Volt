#pragma once

#include "Volt-Renderer/Config.h"
#include "Volt-Renderer/Debug/DebugVertices.h"

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>

#include <RHIModule/Shader/Shader.h>

#include <CoreUtilities/Allocators/FixedSizeLinearAllocator.h>

namespace Volt
{
	class RenderGraph;
	struct RenderView;
	struct ShaderParameterRenderTargetBindings;

	class DebugRenderer
	{
	public:
		VTR_API DebugRenderer();

		VTR_API void ReserveLines(size_t count);
		VTR_API void ReserveBillboards(size_t count);

		VTR_API void DrawLine(const glm::vec3& v0, const glm::vec3& v1, const glm::vec4& color);
		VTR_API void DrawLineSphere(const glm::vec3& center, float radius, const glm::vec4& color);

		VTR_API void DrawBillboard(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color, uint32_t userData = 0);
		VTR_API void DrawBillboard(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color, RefPtr<RHI::Image> texture, uint32_t userData = 0);

		VTR_API void Render(RenderGraph& renderGraph, const RenderView& renderView, RGTextureRef dstTexture, RGTextureRef depthTexture);
		VTR_API void Reset();

		VTR_API size_t GetNumLinesPerLineSphere() const;

		/*
			Custom rendering
		*/
		VTR_API void RenderBillboards(RenderGraph& renderGraph, RefPtr<RHI::Shader> pixelShader, const RenderView& view, const ShaderParameterRenderTargetBindings& renderTargets, bool shouldClear);

	private:
		struct LineSphere
		{
			Vector<glm::vec3> circleXZ;
			Vector<glm::vec3> circleXY;
			Vector<glm::vec3> circleYZ;

		} m_lineSphere;

		struct BillboardInstance
		{
			glm::vec3 position;
			uint32_t userData;
			glm::vec3 size;
			float padding1;
			glm::vec4 color;
		};

		struct BillboardDrawCommand
		{
			glm::vec3 position;
			glm::vec3 size;
			glm::vec4 color;
			uint32_t userData;

			RHI::Image* texture;
		};

		struct BillboardInstancingRange
		{
			uint32_t begin;
			uint32_t count;

			RHI::Image* texture;
		};

		void IniitalizeLineSphere();

		RGBufferRef PrepareBillboardInstancesForRendering(RenderGraph& renderGraph);

		void RenderDebugLines(RenderGraph& renderGraph, const RenderView& view, RGTextureRef dstTexture, RGTextureRef depthTexture);
		void RenderDebugBillboards(RenderGraph& renderGraph, const RenderView& view, RGTextureRef dstTexture, RGTextureRef depthTexture);

		LineVertex* ReserveLineVertices(size_t count);
		void DrawLineWithVertices(LineVertex* vertex0, LineVertex* vertex1, const glm::vec3& v0, const glm::vec3& v1, const glm::vec4& color);

		FixedSizeLinearAllocator<> m_lineVerticesAllocator;
		FixedSizeLinearAllocator<> m_billboardInstanceAllocator;

		Vector<BillboardInstancingRange> m_billboardInstancingRanges;
	};
}
