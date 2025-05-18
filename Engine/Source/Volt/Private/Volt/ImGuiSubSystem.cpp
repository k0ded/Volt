#include "vtpch.h"

#include "Volt/ImGuiSubSystem.h"

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

#include "Volt/Utility/UIUtility.h"

VT_DEFINE_LOG_CATEGORY(LogImGuiSubSystem);

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(ImGuiSubSystem, PostEngine, -1);

	static ConsoleVariable<int32_t> s_imguiEnabled("e.imguiEnabled", 1, "Whether or not imgui is enabled");;

	void ImGuiSubSystem::Initialize()
	{

	} 

	void ImGuiSubSystem::Shutdown()
	{
		m_imguiImplementation = nullptr;
	}

	void ImGuiSubSystem::InitializeImGui()
	{
		VT_LOGC(Trace, LogImGuiSubSystem, "Initializing ImGuiSubSystem!");

		if (s_imguiEnabled.GetValue())
		{
			auto& window = WindowManager::Get().GetMainWindow();

			RHI::ImGuiCreateInfo createInfo{};
			createInfo.swapchain = window.GetSwapchainPtr();
			createInfo.window = window.GetNativeWindow();

			m_imguiImplementation = RHI::ImGuiImplementation::Create(createInfo);
			auto defaultFont = m_imguiImplementation->AddFont("Engine/Fonts/Inter/inter-regular.ttf", 16.f);

			UI::SetFont(FontType::Regular_12, m_imguiImplementation->AddFont("Engine/Fonts/Inter/inter-regular.ttf", 12.f));
			UI::SetFont(FontType::Regular_16, defaultFont);
			UI::SetFont(FontType::Regular_17, m_imguiImplementation->AddFont("Engine/Fonts/Inter/inter-regular.ttf", 17.f));
			UI::SetFont(FontType::Regular_20, m_imguiImplementation->AddFont("Engine/Fonts/Inter/inter-regular.ttf", 20.f));

			UI::SetFont(FontType::Bold_12, m_imguiImplementation->AddFont("Engine/Fonts/Inter/inter-bold.ttf", 12.f));
			UI::SetFont(FontType::Bold_16, m_imguiImplementation->AddFont("Engine/Fonts/Inter/inter-bold.ttf", 16.f));
			UI::SetFont(FontType::Bold_17, m_imguiImplementation->AddFont("Engine/Fonts/Inter/inter-bold.ttf", 17.f));
			UI::SetFont(FontType::Bold_20, m_imguiImplementation->AddFont("Engine/Fonts/Inter/inter-bold.ttf", 20.f));
			UI::SetFont(FontType::Bold_90, m_imguiImplementation->AddFont("Engine/Fonts/Inter/inter-bold.ttf", 90.f));

			m_imguiImplementation->SetDefaultFont(defaultFont);
		}
		VT_LOGC(Trace, LogImGuiSubSystem, "ImGuiSubSystem initialized!");
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
