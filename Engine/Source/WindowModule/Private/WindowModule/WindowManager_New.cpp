#include "windowpch.h"

#include "WindowModule/WindowManager_New.h"
#include "WindowModule/Window_New.h"
#include "WindowModule/WindowLogCategory.h"
#include "WindowModule/Events/WindowEvents_New.h"

#include <EventSystem/EventSystem.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(WindowManager_New, Minimal, PreEngine);

	WindowManager_New::WindowManager_New()
	{
		VT_ASSERT(s_instance == nullptr);
		s_instance = this;
	}

	WindowManager_New::~WindowManager_New()
	{
		s_instance = nullptr;
	}

	WindowHandle WindowManager_New::CreateWindow(const WindowInitializer& initializer)
	{
		VT_LOGC(Trace, LogWindowManagement, "Creating New Window with Title: '{}'", initializer.title);

		WindowHandle newHandle{};
		Unique<Window_New> newWindow = Window_New::Create(initializer, newHandle);

		m_windows[newHandle] = std::move(newWindow);
		return newHandle;
	}

	void WindowManager_New::DestroyWindow(WindowHandle handle)
	{
		if (m_windows.contains(handle))
		{
			VT_LOGC(Trace, LogWindowManagement, "Destroying Window with Title: '{}' ({})", m_windows[handle]->GetTitle(), handle);
		}
		else
		{
			VT_LOGC(Trace, LogWindowManagement, "Failed to destroy Window with handle '{}'! It does not exist!", handle);
		}
	}

	Window_New& WindowManager_New::GetWindow(WindowHandle handle)
	{
		VT_ENSURE_MSG(m_windows.contains(handle), "Window does not exist!");
		return *m_windows.at(handle);
	}

	void WindowManager_New::ProcessMessages()
	{
		for (const auto& [handle, window] : m_windows)
		{
			window->ProcessMessages();
		}
	}

	void WindowManager_New::BeginFrame()
	{
		for (const auto& [handle, window] : m_windows)
		{
			window->BeginFrame();
		}
	}

	void WindowManager_New::Render(float timestep)
	{
		for (const auto& [handle, window] : m_windows)
		{
			WindowRenderEvent_New renderEvent{ *window };
			EventSystem::DispatchEvent(renderEvent);
		}
	}

	void WindowManager_New::Present()
	{
		for (const auto& [handle, window] : m_windows)
		{
			window->Present();
		}
	}

	void WindowManager_New::RepaintWindow(Window_New& window)
	{
		window.BeginFrame();
		
		WindowRenderEvent_New renderEvent{ window };
		EventSystem::DispatchEvent(renderEvent);

		window.Present();
	}

	WindowManager_New& WindowManager_New::Get()
	{
		return *s_instance;
	}

	void WindowManager_New::GetSubSystemDependencies(SubSystemDependencyList& outDependencies)
	{}
}
