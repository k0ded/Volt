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
		uint64_t AddTexture(IntRef<RHI::Image> image);

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

			Vector<IntRef<RHI::Buffer>> vertexBuffers;
			Vector<IntRef<RHI::Buffer>> indexBuffers;
			IntRef<RHI::UniformBuffer>  globalsUniformBuffer;
		};

		void CreatePipeline();
		void UpdateTexture(ImTextureData* textureData);
		IntRef<RHI::RenderPipeline> GetRenderPipeline(RHI::Image& renderTarget);

		void InitalizeMultiViewportSupport();

		IntRef<RHI::SamplerState> m_textureSampler;
		IntRef<RHI::Shader> m_vertexShader;
		IntRef<RHI::Shader> m_pixelShader;

		std::unordered_set<IntRef<RHI::Image>> m_images;

		Vector<Vector<IntRef<RHI::Image>>> m_usedImages;
		Vector<Vector<IntRef<RHI::ImageView>>> m_activeImageViews;

		Map<Window*, RenderContext> m_renderContexts;
		uint32_t m_frameIndex = 0;

		ImGuiRenderTargetManager* m_renderTargetManager = nullptr;
	};
}
