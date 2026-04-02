#include "circuitpch.h"
#include "CircuitManager.h"

#include "Circuit/Window/CircuitWindow.h"

#include "Circuit/Widgets/SliderWidget.h"
#include "Circuit/Widgets/TextWidget.h"
#include "Circuit/Widgets/ButtonWidget.h"
#include "Circuit/Widgets/WindowWidget.h"

#include "Circuit/Widgets/Layout/LayoutWidget.h"

#include "Circuit/CircuitInputHandler.h"

#include <WindowModule/WindowManager_New.h>
#include <WindowModule/Window_New.h>

#include <WindowModule/Events/WindowEvents_New.h>

#include <CoreUtilities/Delegates/Delegate.h>

#include <LogModule/Log.h>
#include <EventSystem/EventSystem.h>

namespace Circuit
{
	CircuitManager::CircuitManager()
	{}

	CircuitManager& CircuitManager::Get()
	{
		VT_ASSERT_MSG(s_Instance.GetRaw() != nullptr, "CircuitManager instance is null");
		return *s_Instance.GetRaw();
	}

	void CircuitManager::Initialize(Ref<Widget> mainWindowWidget, Volt::WindowHandle windowHandle)
	{
		s_Instance = CreateUnique<CircuitManager>();
		s_Instance->Init(mainWindowWidget, windowHandle);
	}

	void CircuitManager::Shutdown()
	{
		s_Instance = nullptr;
	}

	void CircuitManager::Init(Ref<Widget> mainWindowWidget, Volt::WindowHandle windowHandle)
	{
		VT_PROFILE_FUNCTION();
		RegisterEventListeners();
		RegisterWindow(windowHandle);

		InputHandler = CreateUnique<CircuitInputHandler>();
		InputHandler->Init();

		m_windows[windowHandle]->SetWidget(
			CreateWidget(WindowWidget)
			.Content(mainWindowWidget)
			.OnRequestClose_Lambda([windowHandle]()
			{
				Volt::Window_New& window = Volt::WindowManager_New::Get().GetWindow(windowHandle);
				window.Close();
			})

			.OnRequestMinimize_Lambda([windowHandle]()
			{
				Volt::Window_New& window = Volt::WindowManager_New::Get().GetWindow(windowHandle);
				window.Minimize();
			})

			.OnRequestMaximize_Lambda([windowHandle]()
			{
				Volt::Window_New& window = Volt::WindowManager_New::Get().GetWindow(windowHandle);
				if (window.IsMaximized())
				{
					window.Restore();
				}
				else
				{
					window.Maximize();
				}
			})
		);
	}

	void CircuitManager::RegisterEventListeners()
	{
		RegisterListener<Volt::WindowRenderEvent_New>(VT_BIND_EVENT_FN(CircuitManager::OnRenderEvent));
	}

	bool CircuitManager::OnRenderEvent(Volt::WindowRenderEvent_New& e)
	{
		Volt::WindowHandle windowHandle = e.GetWindow().GetHandle();

		if (m_windows.contains(windowHandle))
		{
			m_windows.at(windowHandle)->OnRender();
			return true;
		}

		return false;
	}

	void CircuitManager::RegisterWindow(Volt::WindowHandle handle)
	{
		m_windows[handle] = CreateRef<CircuitWindow>(handle);
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
