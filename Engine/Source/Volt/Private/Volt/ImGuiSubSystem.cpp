#include "vtpch.h"

#include "Volt/ImGuiSubSystem.h"
#include "Volt/Utility/UIUtility.h"

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

#include <CoreUtilities/Time/ScopedTimer.h>

VT_DEFINE_LOG_CATEGORY(LogImGuiSubSystem);

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(ImGuiSubSystem, Minimal, PostEngine, -1);

	static ConsoleVariable<int32_t> s_imguiEnabled("e.imguiEnabled", 1, "Whether or not imgui is enabled");;

	void ImGuiSubSystem::Initialize()
	{

	} 

	void ImGuiSubSystem::Shutdown()
	{
		m_imguiImplementation = nullptr;
	}

	void ImGuiSubSystem::InitializeImGui(bool enableViewports)
	{
		VT_LOGC(Trace, LogImGuiSubSystem, "Initializing ImGuiSubSystem!");

		ScopedTimer timer{};

		if (s_imguiEnabled.GetValue())
		{
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

			UI::SetFont(FontType::Regular_16, imFonts[0]);
			UI::SetFont(FontType::Regular_12, imFonts[1]);
			UI::SetFont(FontType::Regular_17, imFonts[2]);
			UI::SetFont(FontType::Regular_20, imFonts[3]);

			UI::SetFont(FontType::Bold_12, imFonts[4]);
			UI::SetFont(FontType::Bold_16, imFonts[5]);
			UI::SetFont(FontType::Bold_17, imFonts[6]);
			UI::SetFont(FontType::Bold_20, imFonts[7]);
			UI::SetFont(FontType::Bold_90, imFonts[8]);

			m_imguiImplementation->SetDefaultFont(imFonts[0]);
		}
		VT_LOGC(Trace, LogImGuiSubSystem, "ImGuiSubSystem initialized in {} seconds!", timer.GetTime<Time::Seconds>());
	}

	void ImGuiSubSystem::SetupContext()
	{
		if (s_imguiEnabled.GetValue())
		{
			ImGui::SetCurrentContext(m_imguiImplementation->GetContext());
		}
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
