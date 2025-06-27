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

		Vector<std::filesystem::path> fontPaths;
		fontPaths.resize(2);

		fontPaths[0] = "Engine/Fonts/Inter/inter-regular.ttf";
		fontPaths[1] = "Engine/Fonts/Inter/inter-bold.ttf";

		auto imFonts = m_imguiImplementation->AddFonts(fontPaths);

		UI::SetFont(UI::FontType::Regular, imFonts[0]);
		UI::SetFont(UI::FontType::Bold, imFonts[1]);

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
