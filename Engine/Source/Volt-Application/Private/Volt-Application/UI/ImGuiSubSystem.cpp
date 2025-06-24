#include "vtapppch.h"

#include "Volt-Application/UI/ImGuiSubSystem.h"
#include "Volt-Application/UI/UIUtility.h"
#include "Volt-Application/UI/UIFonts.h"

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

#include <CoreUtilities/Time/ScopedTimer.h>
#include <CoreUtilities/Malloc.h>

VT_DEFINE_LOG_CATEGORY(LogImGuiSubSystem);

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(ImGuiSubSystem, Minimal, PostEngine, -1);

	static ConsoleVariable<int32_t> s_imguiEnabled("e.imguiEnabled", 1, "Whether or not imgui is enabled");;

	void ImGuiSubSystem::Initialize()
	{
		constexpr auto imguiMalloc = [](size_t size, void*) -> void*
		{
			return Memory::Malloc(size);
		};

		constexpr auto imguiFree = [](void* ptr, void*)
		{
			Memory::Free(ptr);
		};

		ImGui::SetAllocatorFunctions(imguiMalloc, imguiFree, nullptr);
	}

	void ImGuiSubSystem::Shutdown()
	{
		m_imguiImplementation = nullptr;
	}

	void ImGuiSubSystem::InitializeImGui(bool enableViewports)
	{
		if (!s_imguiEnabled.GetValue())
		{
			VT_LOGC(Trace, LogImGuiSubSystem, "Skipping ImGuiSubsystem initialization because console variable 'e.imguiEnabled' is set to false");
			return;
		}

		VT_LOGC(Trace, LogImGuiSubSystem, "Initializing ImGuiSubSystem!");

		ScopedTimer timer{};

		auto& window = WindowManager::Get().GetMainWindow();

		RHI::ImGuiCreateInfo createInfo{};
		createInfo.swapchain = window.GetSwapchainPtr();
		createInfo.window = window.GetNativeWindow();
		createInfo.enableViewports = enableViewports;

		m_imguiImplementation = RHI::ImGuiImplementation::Create(createInfo);

		Vector<RHI::ImGuiImplementation::FontInfo> fontInfos;
		fontInfos.resize(9);

		fontInfos[0] = { "Engine/Fonts/Inter/inter-regular.ttf", 16.f };
		fontInfos[1] = { "Engine/Fonts/Inter/inter-regular.ttf", 12.f };
		fontInfos[2] = { "Engine/Fonts/Inter/inter-regular.ttf", 17.f };
		fontInfos[3] = { "Engine/Fonts/Inter/inter-regular.ttf", 20.f };
		fontInfos[4] = { "Engine/Fonts/Inter/inter-bold.ttf", 12.f };
		fontInfos[5] = { "Engine/Fonts/Inter/inter-bold.ttf", 16.f };
		fontInfos[6] = { "Engine/Fonts/Inter/inter-bold.ttf", 17.f };
		fontInfos[7] = { "Engine/Fonts/Inter/inter-bold.ttf", 20.f };
		fontInfos[8] = { "Engine/Fonts/Inter/inter-bold.ttf", 90.f };

		auto imFonts = m_imguiImplementation->AddFonts(fontInfos);

		UI::SetFont(UI::FontType::Regular_16, imFonts[0]);
		UI::SetFont(UI::FontType::Regular_12, imFonts[1]);
		UI::SetFont(UI::FontType::Regular_17, imFonts[2]);
		UI::SetFont(UI::FontType::Regular_20, imFonts[3]);

		UI::SetFont(UI::FontType::Bold_12, imFonts[4]);
		UI::SetFont(UI::FontType::Bold_16, imFonts[5]);
		UI::SetFont(UI::FontType::Bold_17, imFonts[6]);
		UI::SetFont(UI::FontType::Bold_20, imFonts[7]);
		UI::SetFont(UI::FontType::Bold_90, imFonts[8]);

		m_imguiImplementation->SetDefaultFont(imFonts[0]);
		VT_LOGC(Trace, LogImGuiSubSystem, "ImGuiSubSystem initialized in {} seconds!", timer.GetTime<Time::Seconds>());
	}

	void ImGuiSubSystem::Begin()
	{
		if (m_imguiImplementation)
		{
			m_imguiImplementation->Begin();
		}
	}

	void ImGuiSubSystem::End()
	{
		if (m_imguiImplementation)
		{
			m_imguiImplementation->End();
		}
	}
}
