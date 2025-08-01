#pragma once

#include "Volt-ImGui/Config.h"

#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Buffers/UniformBuffer.h>
#include <RHIModule/Buffers/CommandBufferSet.h>
#include <RHIModule/Images/SamplerState.h>
#include <RHIModule/Images/Image.h>
#include <RHIModule/Pipelines/RenderPipeline.h>

#include <unordered_set>

struct ImDrawData;
struct ImTextureData;

namespace Volt
{
	class Window;
}

namespace Volt
{
	class VTIMGUI_API ImGuiRenderer
	{
	public:
		ImGuiRenderer(Window* window);
		~ImGuiRenderer();

		void Destroy();

		void Render(ImDrawData* drawData);
		uint64_t AddTexture(RefPtr<RHI::Image> image);

		void RenderImGuiViewport(ImDrawData* drawData, Window* window);
		void AddViewportRenderContext(Window* window);
		void RemoveViewportRenderContext(Window* window);

	private:
		struct RenderContext
		{
			RenderContext(uint32_t numFrames)
				: commandBufferSet(numFrames)
			{}

			RenderContext()
				: commandBufferSet(0)
			{}

			RHI::CommandBufferSet commandBufferSet;

			Vector<RefPtr<RHI::StorageBuffer>> vertexBuffers;
			Vector<RefPtr<RHI::StorageBuffer>> indexBuffers;
			RefPtr<RHI::UniformBuffer>  globalsUniformBuffer;
		};

		void CreatePipeline();
		void UpdateTexture(ImTextureData* textureData);

		void InitalizeMultiViewportSupport();

		RefPtr<RHI::SamplerState> m_textureSampler;
		RefPtr<RHI::RenderPipeline> m_imguiRenderPipeline;

		Window* m_window;

		std::unordered_set<RefPtr<RHI::Image>> m_images;

		Vector<Vector<RefPtr<RHI::Image>>> m_usedImages;

		Map<Window*, RenderContext> m_renderContexts;
		uint32_t m_frameIndex = 0;
	};
}
