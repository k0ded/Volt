#pragma once

#include "Volt-ImGui/Config.h"

#include <RHIModule/Buffers/Buffer.h>
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
	class ImGuiRenderTargetManager;
	class Window;

	class VTIMGUI_API ImGuiRenderer
	{
	public:
		ImGuiRenderer(ImGuiRenderTargetManager* renderTargetManager);
		~ImGuiRenderer();

		void Destroy();

		void Render(ImDrawData* drawData, Window* window, bool shouldUseLoadRTAction);
		void RenderPreviousFrame(Window* window);
		uint64_t AddTexture(RefPtr<RHI::Image> image);

		void RenderImGuiViewport(ImDrawData* drawData, Window* window, bool shouldUseLoadRTAction);
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

			Vector<RefPtr<RHI::Buffer>> vertexBuffers;
			Vector<RefPtr<RHI::Buffer>> indexBuffers;
			RefPtr<RHI::UniformBuffer>  globalsUniformBuffer;
		};

		void CreatePipeline();
		void UpdateTexture(ImTextureData* textureData);
		RefPtr<RHI::RenderPipeline> GetRenderPipeline(RHI::Image& renderTarget);

		void InitalizeMultiViewportSupport();

		RefPtr<RHI::SamplerState> m_textureSampler;
		RefPtr<RHI::Shader> m_vertexShader;
		RefPtr<RHI::Shader> m_pixelShader;

		std::unordered_set<RefPtr<RHI::Image>> m_images;

		Vector<Vector<RefPtr<RHI::Image>>> m_usedImages;
		Vector<Vector<RefPtr<RHI::ImageView>>> m_activeImageViews;

		Map<Window*, RenderContext> m_renderContexts;
		uint32_t m_frameIndex = 0;

		ImGuiRenderTargetManager* m_renderTargetManager = nullptr;
	};
}
