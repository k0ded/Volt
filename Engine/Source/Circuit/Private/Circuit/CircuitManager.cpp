#include "circuitpch.h"
#include "CircuitManager.h"

#include "Circuit/Window/CircuitWindow.h"

#include "Circuit/Widgets/SliderWidget.h"
#include "Circuit/Widgets/TextWidget.h"
#include "Circuit/Widgets/ButtonWidget.h"
#include "Circuit/Widgets/WindowWidget.h"

#include "Circuit/Widgets/Layout/LayoutWidget.h"

#include "Circuit/CircuitInputHandler.h"

#include <WindowModule/WindowManager.h>
#include <WindowModule/Events/WindowEvents.h>
#include <WindowModule/Window.h>

#include <CoreUtilities/Delegates/Delegate.h>

#include <LogModule/Log.h>
#include <EventSystem/EventSystem.h>

namespace Circuit
{
	CircuitManager::CircuitManager()
	{}

	CircuitManager& CircuitManager::Get()
	{
		assert(s_Instance.get() != nullptr && "CircuitManager instance is null");
		return *s_Instance.get();
	}

	void CircuitManager::Initialize(Ref<Widget> mainWindowWidget)
	{
		s_Instance = std::make_unique<CircuitManager>();
		s_Instance->Init(mainWindowWidget);
	}

	void CircuitManager::Shutdown()
	{
		s_Instance = nullptr;
	}

	void CircuitManager::Init(Ref<Widget> mainWindowWidget)
	{
		VT_PROFILE_FUNCTION();
		RegisterEventListeners();
		RegisterWindow(Volt::WindowManager::Get().GetMainWindowHandle());

		InputHandler = CreateScope<CircuitInputHandler>();
		InputHandler->Init();

		m_windows[Volt::WindowManager::Get().GetMainWindowHandle()]->SetWidget(
			CreateWidget(WindowWidget)
			.Content(mainWindowWidget)
			.OnRequestClose_Lambda([]()
		{
			Volt::WindowCloseEvent e{ Volt::WindowManager::Get().GetMainWindow() };
			Volt::EventSystem::DispatchEvent(e);
		})

				.OnRequestMinimize_Lambda([]()
		{
			Volt::WindowManager::Get().GetMainWindow().Minimize();
		})

				.OnRequestMaximize_Lambda([]()
		{
			Volt::Window& mainWindow = Volt::WindowManager::Get().GetMainWindow();
			if (mainWindow.IsMaximized())
			{
				mainWindow.Restore();
			}
			else
			{
				mainWindow.Maximize();
			}
		})
		);
	}

	void CircuitManager::RegisterEventListeners()
	{
		RegisterListener<Volt::WindowRenderEvent>(VT_BIND_EVENT_FN(CircuitManager::OnRenderEvent));
	}

	bool CircuitManager::OnRenderEvent(Volt::WindowRenderEvent& e)
	{
		for (auto& [windowHandle, window] : m_windows)
		{
			window->OnRender();
		}

		return false;
	}

	void CircuitManager::RegisterWindow(Volt::WindowHandle handle)
	{
		m_windows.emplace(handle, CreateScope<CircuitWindow>(handle));
	}

	int32_t CircuitManager::TestingStaticDelegates(float aParameter)
	{
		VT_LOG(Warning, "I WAS CALLED FROM A STATIC DELEGATE!!! FloatParameter: {0}", aParameter);

		return static_cast<int32_t>(aParameter * 2);
	}

	int32_t CircuitManager::TestingRawDelegates(float aParameter)
	{
		VT_LOG(Warning, "I WAS CALLED FROM A RAW DELEGATE!!! FloatParameter: {0}, WindowCount: {1}", aParameter, m_windows.size());

		return static_cast<int32_t>(aParameter / 2);
	}

	void CircuitManager::Update()
	{}

	CIRCUIT_API Vector<Weak<CircuitWindow>> CircuitManager::GetWindows()
	{
		Vector<Weak<CircuitWindow>> windows;
		for (auto& [windowHandle, window] : m_windows)
		{
			windows.push_back(window);
		}
		return windows;
	}

	//CircuitWindow& CircuitManager::OpenWindow(OpenWindowParams& params)
	//{
	//	//const size_t startWindowCount = m_windows.size();
	//	//BroadcastTellEvent(OpenWindowTellEvent(params));
	//	//assert(m_windows.size() == (startWindowCount + 1) && "Failed to open window.");

	//	return *((--m_windows.end())->second);
	//}
}
