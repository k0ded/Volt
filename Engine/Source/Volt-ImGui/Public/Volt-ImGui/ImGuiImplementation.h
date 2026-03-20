#pragma once

#include "Volt-ImGui/Config.h"
#include "Volt-ImGui/ImGuiPlatform.h"
#include "Volt-ImGui/ImGuiRenderer.h"
#include "Volt-ImGui/ImGuiRenderTargetManager.h"

#include <RHIModule/Graphics/Swapchain.h>
#include <RHIModule/Images/Image.h>

#include <CoreUtilities/Pointers/Unique.h>

struct ImFont;
struct ImGuiContext;
struct ImFontAtlas;

using ImTextureID = uint64_t;

namespace Volt
{
	struct ImGuiCreateInfo
	{
		Window* window;
		bool enableViewports = true;
	};

	class VTIMGUI_API ImGuiImplementation
	{
	public:
		ImGuiImplementation(const ImGuiCreateInfo& createInfo);
		~ImGuiImplementation();

		void Begin();
		void End();

		void RenderPreviousFrameContextStack();

		void PushNewContext();
		void PopContext();

		void SetDefaultFont(ImFont* font);
		ImFont* AddFont(const std::filesystem::path& fontPath);
		Vector<ImFont*> AddFonts(const Vector<std::filesystem::path>& fontPaths);

		ImTextureID GetTextureID(IntRef<RHI::Image> image, int32_t mipIndex);

	private:
		struct ContextData
		{
			ImGuiContext* context;
			Ref<ImGuiPlatform> platform;
			Ref<ImGuiRenderer> renderer;
		};

		struct PerWindowData
		{
			IntRef<RHI::Image> renderTarget;
		};

		void Initialize();
		void CreateCopyGlobalsUniformBuffer();
		ContextData CreateAndInitializeNewContext();

		Ref<ImGuiRenderer> GetActiveRenderer() const { return m_contextStack.back().renderer; }
		Ref<ImGuiPlatform> GetActivePlatform() const { return m_contextStack.back().platform; }

		ImGuiCreateInfo m_createInfo;

		ImFont* m_defaultFont = nullptr;
		ImFontAtlas* m_sharedFontAtlas = nullptr;

		Vector<ContextData> m_contextStack;

		IntRef<RHI::UniformBuffer> m_copyGlobalsUniformBuffer;

		Unique<ImGuiRenderTargetManager> m_renderTargetManager;
	};
}
