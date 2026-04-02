#include "vtapppch.h"

#include "Volt-Application/UI/ImGuiSubSystem.h"
#include "Volt-Application/UI/UIUtility.h"
#include "Volt-Application/UI/UIFonts.h"
#include "Volt-Application/BaseApplication.h"

#include <CoreUtilities/ConsoleVariableRegistry.h>
#include <Volt-ImGui/ImGuiImplementation.h>

#include <RHIModule/Graphics/DeviceQueue.h>
#include <RHIModule/Graphics/GraphicsDevice.h>
#include <RHIModule/Graphics/GraphicsContext.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

#include <EventSystem/ApplicationEvents.h>
#include <EventSystem/EventSystem.h>

#include <CoreUtilities/Time/ScopedTimer.h>
#include <CoreUtilities/Malloc.h>

VT_DEFINE_LOG_CATEGORY(LogImGuiSubSystem);

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(ImGuiSubSystem, Minimal, PostEngine);

	static ConsoleVariable<int32_t> s_imguiEnabled("e.imguiEnabled", 1, "Whether or not imgui is enabled");;

	ImGuiSubSystem::ImGuiSubSystem()
	{
		RegisterListener<AppBeginFrameEvent>(VT_BIND_EVENT_FN(ImGuiSubSystem::OnAppBeginFrameEvent));
		RegisterListener<AppPresentFrameEvent>(VT_BIND_EVENT_FN(ImGuiSubSystem::OnAppPresentFrameEvent));
	}

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

		ImGuiCreateInfo createInfo{};
		createInfo.enableViewports = enableViewports;
		createInfo.window = &window;

		m_imguiImplementation = CreateRef<ImGuiImplementation>(createInfo);

		Vector<Filesystem::Path> fontPaths;
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
			m_isWithinImGuiUpdate = true;
		}
	}

	void ImGuiSubSystem::End()
	{
		if (m_imguiImplementation)
		{
			m_isWithinImGuiUpdate = false;
			m_imguiImplementation->End();
		}
	}

	void ImGuiSubSystem::EnterBlockingContext(std::function<void()> onEnterCallback)
	{
		m_isBlockingActive = true;
		m_imguiImplementation->PushNewContext();

		bool wasWithinFrameBeforeEntering = m_isWithinFrame;

		if (wasWithinFrameBeforeEntering)
		{
			WindowManager::Get().Present();
		}

		bool hasCalledCallback = false;

		while (m_isBlockingActive)
		{
			WindowManager::Get().BeginFrame();

			BaseApplication::Get().Tick();

			AppPreRenderEvent preRenderEvent(BaseApplication::Get().GetFrameIndex());
			EventSystem::DispatchEvent(preRenderEvent);

			m_imguiImplementation->Begin();

			if (!hasCalledCallback && onEnterCallback)
			{
				onEnterCallback();
				hasCalledCallback = true;
			}

			AppImGuiBlockingUpdateEvent event{};
			EventSystem::DispatchEvent(event);

			m_imguiImplementation->RenderPreviousFrameContextStack();
			m_imguiImplementation->End();

			WindowManager::Get().Present();
		}

		m_imguiImplementation->PopContext();

		RHI::GraphicsContext::GetDevice()->GetDeviceQueue(RHI::QueueType::Graphics)->WaitForQueue();

		if (wasWithinFrameBeforeEntering)
		{
			WindowManager::Get().BeginFrame();
		}
	}

	void ImGuiSubSystem::ExitBlockingContext()
	{
		m_isBlockingActive = false;
	}

	ImTextureID ImGuiSubSystem::GetTextureID(IntRef<RHI::Image> image, int32_t mipIndex /*= -1*/)
	{
		return m_imguiImplementation->GetTextureID(image, mipIndex);
	}

	bool ImGuiSubSystem::OnAppBeginFrameEvent(AppBeginFrameEvent& e)
	{
		m_isWithinFrame = true;
		return false;
	}

	bool ImGuiSubSystem::OnAppPresentFrameEvent(AppPresentFrameEvent& e)
	{
		m_isWithinFrame = false;
		return false;
	}
}
