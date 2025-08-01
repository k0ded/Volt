#pragma once

#include "Volt-ImGui/Config.h"
#include "Volt-ImGui/ImGuiPlatform.h"
#include "Volt-ImGui/ImGuiRenderer.h"

#include <RHIModule/Graphics/Swapchain.h>
#include <RHIModule/Images/Image.h>

struct ImFont;

using ImTextureID = uint64_t;

namespace Volt
{
	struct ImGuiCreateInfo2
	{
		Window* window;
		bool enableViewports = true;
	};

	class VTIMGUI_API ImGuiImplementation
	{
	public:
		ImGuiImplementation(const ImGuiCreateInfo2& createInfo);
		~ImGuiImplementation();

		void Begin();
		void End();

		void SetDefaultFont(ImFont* font);
		ImFont* AddFont(const std::filesystem::path& fontPath);
		Vector<ImFont*> AddFonts(const Vector<std::filesystem::path>& fontPaths);

		ImTextureID GetTextureID(RefPtr<RHI::Image> image, int32_t mipIndex);

	private:
		void Initialize();

		ImGuiCreateInfo2 m_createInfo;

		ImFont* m_defaultFont = nullptr;
		Scope<ImGuiPlatform> m_platform;
		Scope<ImGuiRenderer> m_renderer;
	};
}
