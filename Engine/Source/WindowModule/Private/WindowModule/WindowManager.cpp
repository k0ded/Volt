#include "windowpch.h"
#include "WindowManager.h"

#include "WindowLogCategory.h"

#include "Window.h"

#include "WindowModule/Monitor.h"
#include "WindowModule/Events/WindowEvents.h"

#include "EventSystem/EventSystem.h"

#include <GLFW/glfw3.h>

#include <LogModule/Log.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(WindowManager, Minimal, PreEngine);

	static bool s_glfwIsInitialized = false;

	inline static void GLFWErrorCallback(int error, const char* description)
	{
		VT_LOGC(Error, LogWindowManagement, "GLFW Error ({0}): {1}", error, description);
	}

	WindowManager::WindowManager()
	{
		VT_ASSERT(!s_instance);
		s_instance = this;
	}

	WindowManager::~WindowManager()
	{
		s_instance = nullptr;
	}

	void WindowManager::Initialize()
	{
		VT_LOGC(Trace, LogWindowManagement, "Initializing WindowManager");
		InitializeGLFW();
		InitializeMonitors();
	}

	void WindowManager::Shutdown()
	{
		VT_LOGC(Trace, LogWindowManagement, "Shutting down WindowManager");
		ShutdownGLFW();
	}

	void WindowManager::CreateMainWindow(const WindowProperties& windowProperties)
	{
		m_mainWindowHandle = CreateNewWindow(windowProperties);
	}

	void WindowManager::DestroyMainWindow()
	{
		if (m_mainWindowHandle != 0)
		{
			DestroyWindow(m_mainWindowHandle);
		}
	}

	WindowManager& WindowManager::Get()
	{
		return *s_instance;
	}

	void WindowManager::GetSubSystemDependencies(SubSystemDependencyList& outDependencies)
	{
		outDependencies.AddDependency<Log>();
	}

	void WindowManager::InitializeMonitors()
	{
		int32_t numMonitors = 0;
		GLFWmonitor** monitors = glfwGetMonitors(&numMonitors);

		for (int32_t i = 0; i < numMonitors; ++i)
		{
			AddMonitor(monitors[i]);
		}
	
		glfwSetMonitorCallback([](GLFWmonitor* nativeMonitor, int32_t event) 
		{
			auto& windowManager = WindowManager::Get();

			if (event == GLFW_CONNECTED)
			{
				Ref<Monitor> monitor = windowManager.TryGetMonitor(nativeMonitor);
				if (monitor == nullptr)
				{
					monitor = windowManager.AddMonitor(nativeMonitor);
				}

				VT_ENSURE_MSG(monitor != nullptr, "\"monitor\" variable should not be null at this point!");

				MonitorConnectedEvent connectedEvent(*monitor);
				EventSystem::DispatchEvent(connectedEvent);
			}
			else if (event == GLFW_DISCONNECTED)
			{
				Ref<Monitor> monitor = windowManager.TryGetMonitor(nativeMonitor);
				if (monitor)
				{
					MonitorDisconnectedEvent disconnectedEvent(*monitor);
					EventSystem::DispatchEvent(disconnectedEvent);

					windowManager.RemoveMonitor(monitor);
				}
			}
		});
	}

	const WindowHandle WindowManager::CreateNewWindow(const WindowProperties& windowProperties)
	{
		VT_LOGC(Trace, LogWindowManagement, "Creating New Window with Title: '{0}'", windowProperties.title);

		Scope<Window> window = Window::Create(windowProperties, m_forceSDR);
		WindowHandle handle{};

		m_windows[handle] = std::move(window);
		return handle;
	}

	void WindowManager::DestroyWindow(const WindowHandle handle)
	{
		if (m_windows.contains(handle))
		{
			VT_LOGC(Trace, LogWindowManagement, "Destroying Window with Title: '{0}'", m_windows[handle]->GetTitle());
			m_windows.erase(handle);
		}
		else
		{
			VT_LOGC(Trace, LogWindowManagement, "Failed to Window with Handle: '{0}'", handle);
		}
	}

	void WindowManager::DestroyWindow(Window& window)
	{
		WindowHandle windowHandle = 0;

		for (const auto& [handle, wnd] : m_windows)
		{
			if (&window == wnd.get())
			{
				windowHandle = handle;
				break;
			}
		}

		if (windowHandle != 0)
		{
			VT_LOGC(Trace, LogWindowManagement, "Destroying Window with Title: '{0}'", window.GetTitle());
			m_windows.erase(windowHandle);
		}
	}

	void WindowManager::BeginFrame()
	{
		for (const auto& [handle, window] : m_windows)
		{
			window->BeginFrame();
		}
	}

	void WindowManager::Render(float timestep)
	{
		for (const auto& [handle, window] : m_windows)
		{
			window->Render(timestep);
		}
	}

	void WindowManager::Present()
	{
		for (const auto& [handle, window] : m_windows)
		{
			window->Present();
		}

		glfwPollEvents();
	}

	WindowHandle WindowManager::GetMainWindowHandle() const
	{
		return m_mainWindowHandle;
	}

	Window& WindowManager::GetMainWindow() const
	{
		return *m_windows.at(m_mainWindowHandle);
	}

	Window& WindowManager::GetWindow(const WindowHandle handle) const
	{
		return *m_windows.at(handle);
	}

	void WindowManager::InitializeGLFW()
	{
		VT_LOGC(Trace, LogWindowManagement, "Initializing GLFW");
		if (!s_glfwIsInitialized)
		{
			s_glfwIsInitialized = true;
			if (!glfwInit())
			{
				VT_LOGC(Critical, LogWindowManagement, "Failed to initialize GLFW!");
			}

			glfwSetErrorCallback(GLFWErrorCallback);
		}
	}

	void WindowManager::ShutdownGLFW()
	{
		VT_LOGC(Trace, LogWindowManagement, "Shutting Down GLFW");
		glfwTerminate();

		s_glfwIsInitialized = false;
	}

	Ref<Monitor> WindowManager::TryGetMonitor(GLFWmonitor* nativeMonitor)
	{
		for (auto monitor : m_monitors)
		{
			if (monitor->GetNativeMonitor() == nativeMonitor)
			{
				return monitor;
			}
		}

		return nullptr;
	}

	Ref<Monitor> WindowManager::AddMonitor(GLFWmonitor* nativeMonitor)
	{
		Ref<Monitor> monitor = CreateRef<Monitor>(nativeMonitor);
		m_monitors.emplace_back(monitor);

		return monitor;
	}

	void WindowManager::RemoveMonitor(Ref<Monitor> monitor)
	{
		for (auto it = m_monitors.begin(); it != m_monitors.end(); ++it)
		{
			if ((*it) == monitor)
			{
				m_monitors.erase(it);
				return;
			}
		}
	}
}
