#include "windowpch.h"
#include "Window.h"

#include "WindowLogCategory.h"

#include "Utilities/DDSUtility.h"

#include "Events/WindowEvents.h"

#include <InputModule/Events/KeyboardEvents.h>
#include <InputModule/Events/MouseEvents.h>  

#include <EventSystem/EventSystem.h>

#include <CoreUtilities/Profiling/Profiling.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <LogModule/Log.h>

namespace Volt
{
	Window::Window(const WindowProperties& properties)
	{
		m_data.height = properties.height;
		m_data.width = properties.width;
		m_data.title = properties.title;
		m_data.vsync = properties.vsync;
		m_data.windowMode = properties.windowMode;
		m_data.iconPath = properties.iconPath;
		m_data.cursorPath = properties.cursorPath;

		m_properties = properties;

		Invalidate();
		CreateDefaultCursors();

		if (!m_data.cursorPath.empty())
		{
			ReplaceCursor(CursorType::Arrow, m_data.cursorPath);
		}
	}

	Window::~Window()
	{
		Shutdown();
	}

	void Window::Shutdown()
	{
		m_swapchain = nullptr;
		Release();

		for (size_t i = 0; i < static_cast<size_t>(CursorType::Num); ++i)
		{
			glfwDestroyCursor(m_cursors[i]);
		}
	}

	void Window::Invalidate()
	{
		VT_PROFILE_FUNCTION();

		if (m_window)
		{
			Release();
		}

		// Setup window hints
		{
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			glfwWindowHint(GLFW_TITLEBAR, (m_properties.useTitlebar && !m_properties.useCustomTitlebar) ? GLFW_TRUE : GLFW_FALSE);
			glfwWindowHint(GLFW_AUTO_ICONIFY, false);

			glfwWindowHint(GLFW_VISIBLE, m_properties.createAsVisible);
			glfwWindowHint(GLFW_FOCUSED, m_properties.createAsFocused);
			glfwWindowHint(GLFW_FOCUS_ON_SHOW, m_properties.focusOnShow);
			glfwWindowHint(GLFW_DECORATED, m_properties.createAsDecorated);
			glfwWindowHint(GLFW_FLOATING, m_properties.createAsAlwaysOnTop);
		}

		GLFWmonitor* primaryMonitor = nullptr;

		if (m_data.windowMode != WindowMode::Windowed)
		{
			primaryMonitor = glfwGetPrimaryMonitor();
		}

		int32_t createWidth = (uint32_t)m_data.width;
		int32_t createHeight = (uint32_t)m_data.height;

		if (primaryMonitor)
		{
			const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
			createWidth = mode->width;
			createHeight = mode->height;
		}

		m_window = glfwCreateWindow(createWidth, createHeight, m_data.title.c_str(), primaryMonitor, nullptr);
		m_windowHandle = glfwGetWin32Window(m_window);

		// If we have no title bar, or use a custom one, we need to add the window frame size to
		// the window size, this is to make sure the swapchain is created in the correct size.
		if (!m_properties.useTitlebar || (m_properties.useTitlebar && m_properties.useCustomTitlebar))
		{
			int32_t actualWidth;
			int32_t actualHeight;

			glfwGetWindowSize(m_window, &actualWidth, &actualHeight);

			int32_t left, right, bottom, top;
			glfwGetWindowFrameSize(m_window, &left, &top, &right, &bottom);

			m_data.width = static_cast<uint32_t>(actualWidth + left + right);
			m_data.height = static_cast<uint32_t>(actualHeight + bottom + top);

			// For some reason we need to force a resize of the window for
			// the titlebar to actually disappear.
			glfwSetWindowSize(m_window, static_cast<int32_t>(createWidth + 1), static_cast<int32_t>(createHeight + 1));
			glfwSetWindowSize(m_window, static_cast<int32_t>(createWidth), static_cast<int32_t>(createHeight));
		}

		if (!m_data.iconPath.empty() && std::filesystem::exists(m_data.iconPath))
		{
			SetIcon(m_data.iconPath);
		}

		bool isRawMouseMotionSupported = glfwRawMouseMotionSupported();
		if (isRawMouseMotionSupported)
		{
			glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		}

		if (m_data.windowMode == WindowMode::Fullscreen)
		{
			m_isFullscreen = true;
		}

		if (!m_hasBeenInitialized)
		{
			VT_PROFILE_SCOPE("Create Swapchain");

			RHI::SwapchainCreateInfo createInfo{};
			createInfo.width = m_data.width;
			createInfo.height = m_data.height;
			createInfo.platformWindow = m_window;
			createInfo.useHDRIfAvailable = false;
			createInfo.enableVSync = m_data.vsync;

			m_swapchain = RHI::Swapchain::Create(createInfo);
			m_hasBeenInitialized = true;
		}

		if (m_data.windowMode != WindowMode::Windowed)
		{
			SetWindowMode(m_data.windowMode, true);
		}

		glfwSetWindowUserPointer(m_window, this);

		glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int32_t width, int32_t height)
		{
			Window& voltWindow = *(Window*)glfwGetWindowUserPointer(window);

			if (!voltWindow.m_shouldSkipDispatchResizeEvent)
			{
				int32_t x, y;
				glfwGetWindowPos(window, &x, &y);

				WindowResizeEvent event(voltWindow, (uint32_t)x, (uint32_t)y, width, height);
				EventSystem::DispatchEvent(event);

				voltWindow.m_data.width = width;
				voltWindow.m_data.height = height;
			}

			voltWindow.m_shouldSkipDispatchResizeEvent = false;
		});

		glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window)
		{
			Window& voltWindow = *(Window*)glfwGetWindowUserPointer(window);

			WindowCloseEvent event{ voltWindow };
			EventSystem::DispatchEvent(event);
		});

		if (m_properties.useTitlebar && m_properties.useCustomTitlebar)
		{
			glfwSetTitlebarHitTestCallback(m_window, [](GLFWwindow* window, int x, int y, int* hit)
			{
				Window& voltWindow = *(Window*)glfwGetWindowUserPointer(window);

				WindowTitlebarHittestEvent event{ voltWindow, x, y, *hit };
				EventSystem::DispatchEvent(event);
			});
		}

		glfwSetKeyCallback(m_window, [](GLFWwindow* window, int32_t key, int32_t scanCode, int32_t action, int32_t modifiers)
		{
			if (key == -1)
			{
				return;
			}

			Window& voltWindow = *(Window*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(voltWindow, key, scanCode, 0, modifiers);
					EventSystem::DispatchEvent(event);
					break;
				}

				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(voltWindow, key, scanCode, modifiers);
					EventSystem::DispatchEvent(event);
					break;
				}

				case GLFW_REPEAT:
				{
					KeyPressedEvent event(voltWindow, key, scanCode, 1, modifiers);
					EventSystem::DispatchEvent(event);
					break;
				}
			}
		});		

		glfwSetCharCallback(m_window, [](GLFWwindow* window, uint32_t character)
		{
			Window& voltWindow = *(Window*)glfwGetWindowUserPointer(window);

			KeyTypedEvent event(voltWindow, character);
			EventSystem::DispatchEvent(event);
		});

		glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int modifiers)
		{
			Window& voltWindow = *(Window*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(voltWindow, button, modifiers);
					EventSystem::DispatchEvent(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(voltWindow, button, modifiers);
					EventSystem::DispatchEvent(event);
					break;
				}
			}
		});

		glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			Window& voltWindow = *(Window*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent event(voltWindow, (float)xOffset, (float)yOffset);
			EventSystem::DispatchEvent(event);
		});

		glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xPos, double yPos)
		{
			Window& voltWindow = *(Window*)glfwGetWindowUserPointer(window);

			MouseMovedEvent event(voltWindow, (float)xPos, (float)yPos);
			EventSystem::DispatchEvent(event);
		});

		glfwSetDropCallback(m_window, [](GLFWwindow* window, int32_t count, const char** paths)
		{
			Window& voltWindow = *(Window*)glfwGetWindowUserPointer(window);

			WindowDragDropEvent event(voltWindow, count, paths);
			EventSystem::DispatchEvent(event);
		});

		glfwSetWindowFocusCallback(m_window, [](GLFWwindow* window, int32_t focused)
		{
			Window& voltWindow = *(Window*)glfwGetWindowUserPointer(window);

			WindowFocusChangedEvent event(voltWindow, focused == GLFW_TRUE);
			EventSystem::DispatchEvent(event);
		});

		glfwSetCursorEnterCallback(m_window, [](GLFWwindow* window, int32_t entered) 
		{
			Window& voltWindow = *(Window*)glfwGetWindowUserPointer(window);

			WindowCursorEnteredEvent event(voltWindow, entered == GLFW_TRUE);
			EventSystem::DispatchEvent(event);
		});

		m_eventListener = CreateScope<WindowEventListener>(m_window);
	}

	void Window::Release()
	{
		m_eventListener.reset();

		if (m_window)
		{
			glfwDestroyWindow(m_window);
			m_window = nullptr;
		}
	}

	void Window::SetWindowMode(WindowMode aWindowMode, bool first)
	{
		m_data.windowMode = aWindowMode;

		switch (aWindowMode)
		{
			case WindowMode::Fullscreen:
			{
				const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

				glfwSetWindowAttrib(m_window, GLFW_DECORATED, false);
				glfwSetWindowAttrib(m_window, GLFW_TITLEBAR, false);
				glfwSetWindowAttrib(m_window, GLFW_AUTO_ICONIFY, true);
				glfwSetWindowAttrib(m_window, GLFW_RESIZABLE, false);

				glfwWindowHint(GLFW_RED_BITS, mode->redBits);
				glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
				glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
				glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

				glfwSetWindowMonitor(m_window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, GLFW_DONT_CARE);

				if (first)
				{
					Resize(mode->width, mode->height);
				}
				else
				{
					//const auto [wx, wy] = GetPosition();
					//WindowResizeEvent resizeWindow{ static_cast<uint32_t>(wx), static_cast<uint32_t>(wy), static_cast<uint32_t>(mode->width), static_cast<uint32_t>(mode->height) };
					//Application::Get().OnEvent(resizeWindow);
				}

				break;
			}

			case WindowMode::Windowed:
			{
				const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

				glfwSetWindowAttrib(m_window, GLFW_DECORATED, true);
				glfwSetWindowAttrib(m_window, GLFW_TITLEBAR, true);
				glfwSetWindowAttrib(m_window, GLFW_AUTO_ICONIFY, false);
				glfwSetWindowAttrib(m_window, GLFW_RESIZABLE, true);

				glfwSetWindowMonitor(m_window, nullptr, 0, 0, m_properties.width, m_properties.height, GLFW_DONT_CARE);

				const int32_t xPos = (int32_t)((mode->width / 2) - (m_properties.width / 2));
				const int32_t yPos = (int32_t)((mode->height / 2) - (m_properties.height / 2));

				glfwSetWindowPos(m_window, xPos, yPos);

				m_isFullscreen = false;

				if (first)
				{
					Resize(m_properties.width, m_properties.height);
				}
				else
				{
					/*WindowResizeEvent resizeWindow{ static_cast<uint32_t>(xPos), static_cast<uint32_t>(yPos), m_properties.width, m_properties.height };
					Application::Get().OnEvent(resizeWindow);*/
				}
				break;
			}

			case WindowMode::Borderless:
			{
				glfwSetWindowAttrib(m_window, GLFW_DECORATED, false);
				glfwSetWindowAttrib(m_window, GLFW_TITLEBAR, false);
				glfwSetWindowAttrib(m_window, GLFW_AUTO_ICONIFY, false);
				glfwSetWindowAttrib(m_window, GLFW_RESIZABLE, false);

				const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
				glfwSetWindowMonitor(m_window, nullptr, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);

				m_isFullscreen = false;

				if (first)
				{
					Resize(mode->width, mode->height);
				}
				else
				{
					//const auto [wx, wy] = GetPosition();
					/*WindowResizeEvent resizeWindow{ static_cast<uint32_t>(wx), static_cast<uint32_t>(wy), static_cast<uint32_t>(mode->width), static_cast<uint32_t>(mode->height) };
					Application::Get().OnEvent(resizeWindow);*/
				}
				break;
			}
		}
	}

	void Window::SetVsync(bool aState)
	{
		m_data.vsync = aState;
	}

	void Window::SetTitle(const std::string& title)
	{
		m_data.title = title;
		glfwSetWindowTitle(m_window, m_data.title.c_str());
	}

	void Window::SetIcon(const std::filesystem::path& path)
	{
		m_data.iconPath = path;

		auto textureData = DDSUtility::GetRawDataFromDDS(m_data.iconPath);

		GLFWimage image;
		image.width = (int32_t)textureData.width;
		image.height = (int32_t)textureData.height;
		image.pixels = textureData.dataBuffer.As<uint8_t>();

		glfwSetWindowIcon(m_window, 1, &image);

		textureData.dataBuffer.Release();
	}

	void Window::EnableMousePassthrough(bool state)
	{
		glfwSetWindowAttrib(m_window, GLFW_MOUSE_PASSTHROUGH, state);
	}

	void Window::BeginFrame()
	{
		WindowBeginFrameEvent beginFrameEvent(*this);
		EventSystem::DispatchEvent(beginFrameEvent);
		m_swapchain->BeginFrame();

		m_frameHasStarted = true;
	}

	void Window::Render(float timestep)
	{
		WindowRenderEvent renderEvent(*this, timestep);
		EventSystem::DispatchEvent(renderEvent);
	}

	void Window::Present()
	{
		if (m_frameHasStarted)
		{
			m_swapchain->Present();

			WindowPresentFrameEvent presentFrameEvent(*this);
			EventSystem::DispatchEvent(presentFrameEvent);
		}

		m_frameHasStarted = false;
	}

	void Window::Resize(uint32_t aWidth, uint32_t aHeight)
	{
		if (aWidth == 0 && aHeight == 0)
		{
			int tempWidth = 0, tempHeight = 0;
			while (tempWidth == 0 && tempHeight == 0)
			{
				glfwGetFramebufferSize(m_window, &tempWidth, &tempHeight);
				glfwWaitEvents();
			}
		}

		// No resize required
		if (m_data.width == aWidth && m_data.height == aHeight)
		{
			return;
		}

		m_data.width = aWidth;
		m_data.height = aHeight;

		// Make sure that we don't end up in a recursive resize.
		m_shouldSkipDispatchResizeEvent = true;
		//glfwSetWindowSize(m_window, static_cast<int32_t>(aWidth), static_cast<int32_t>(aHeight));

		m_swapchain->Resize(aWidth, aHeight, m_data.vsync);
	}

	void Window::SetViewportSize(uint32_t width, uint32_t height)
	{
		m_viewportWidth = width;
		m_viewportHeight = height;
	}

	void Window::SetPosition(int32_t x, int32_t y)
	{
		glfwSetWindowPos(m_window, x, y);
	}

	void Window::Maximize() const
	{
		glfwMaximizeWindow(m_window);
	}

	void Window::Minimize() const
	{
		glfwIconifyWindow(m_window);
	}

	void Window::Restore() const
	{
		glfwRestoreWindow(m_window);
	}

	void Window::Show() const
	{
		glfwShowWindow(m_window);
	}

	void Window::Hide() const
	{
		glfwHideWindow(m_window);
	}

	void Window::ShowCursor(bool state)
	{
		glfwSetInputMode(m_window, GLFW_CURSOR, state ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}

	void Window::Focus() const
	{
		glfwFocusWindow(m_window);
	}

	bool Window::IsFocused() const
	{
		int32_t focused = glfwGetWindowAttrib(m_window, GLFW_FOCUSED);
		return focused != 0;
	}

	bool Window::IsHovered() const
	{
		return glfwGetWindowAttrib(m_window, GLFW_HOVERED) != 0;
	}

	bool Window::IsMaximized() const
	{
		return glfwGetWindowAttrib(m_window, GLFW_MAXIMIZED) == GLFW_MAXIMIZED;
	}

	bool Window::IsMinimized() const
	{
		return glfwGetWindowAttrib(m_window, GLFW_ICONIFIED) != 0;
	}

	bool Window::IsCursorEnabled() const
	{
		return glfwGetInputMode(m_window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL;
	}

	void Window::ReplaceCursor(CursorType cursorType, const std::filesystem::path& path)
	{
		if (!std::filesystem::exists(path))
		{
			return;
		}

		auto textureData = DDSUtility::GetRawDataFromDDS(path);

		GLFWimage image;
		image.width = (int32_t)textureData.width;
		image.height = (int32_t)textureData.height;
		image.pixels = textureData.dataBuffer.As<uint8_t>();

		GLFWcursor* cursor = glfwCreateCursor(&image, 0, 0);
		glfwDestroyCursor(m_cursors[static_cast<size_t>(CursorType::Arrow)]);

		m_cursors[static_cast<size_t>(CursorType::Arrow)] = cursor;

		textureData.dataBuffer.Release();
		glfwSetCursor(m_window, cursor);
	}

	void Window::SetCursor(CursorType cursorType)
	{
		glfwSetCursor(m_window, m_cursors[static_cast<size_t>(cursorType)]);
	}

	void Window::SetOpacity(float opacity) const
	{
		opacity = std::clamp(opacity, 0.f, 1.f);
		glfwSetWindowOpacity(m_window, opacity);
	}

	std::string_view Window::GetClipboard() const
	{
		return glfwGetClipboardString(m_window);
	}

	void Window::SetClipboard(std::string_view string)
	{
		glfwSetClipboardString(m_window, string.data());
	}

	const std::pair<float, float> Window::GetPosition() const
	{
		int32_t x, y;
		glfwGetWindowPos(m_window, &x, &y);

		return { (float)x, (float)y };
	}

	const std::pair<int32_t, int32_t> Window::GetFramebufferSize() const
	{
		int32_t x, y;
		glfwGetFramebufferSize(m_window, &x, &y);

		return { x, y };
	}

	const std::pair<float, float> Window::GetCursorPos() const
	{
		double xpos, ypos;
		glfwGetCursorPos(m_window, &xpos, &ypos);

		return { static_cast<float>(xpos), static_cast<float>(ypos) };
	}

	const float Window::GetOpacity() const
	{
		return glfwGetWindowOpacity(m_window);
	}

	const float Window::GetTime() const
	{
		return static_cast<float>(glfwGetTime());
	}

	WINDOWMODULE_API const std::string& Window::GetTitle()
	{
		return m_data.title;
	}

	Scope<Window> Window::Create(const WindowProperties& aProperties)
	{
		return CreateScope<Window>(aProperties);
	}

	void Window::CreateDefaultCursors()
	{
		m_cursors[static_cast<size_t>(CursorType::Arrow)] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
		m_cursors[static_cast<size_t>(CursorType::TextInput)] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
		m_cursors[static_cast<size_t>(CursorType::ResizeNS)] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
		m_cursors[static_cast<size_t>(CursorType::ResizeEW)] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
		m_cursors[static_cast<size_t>(CursorType::ResizeAll)] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
		m_cursors[static_cast<size_t>(CursorType::ResizeNESW)] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
		m_cursors[static_cast<size_t>(CursorType::ResizeNWSE)] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
		m_cursors[static_cast<size_t>(CursorType::NotAllowed)] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
		m_cursors[static_cast<size_t>(CursorType::Hand)] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

		glfwSetCursor(m_window, m_cursors[0]);
	}

	Window::WindowEventListener::WindowEventListener(GLFWwindow* glfwWindow)
		: m_window(glfwWindow)
	{
		RegisterListener<SetShowCursorEvent>([this](SetShowCursorEvent& event)
		{
			glfwSetInputMode(m_window, GLFW_CURSOR, event.ShouldShow() ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
			return false;
		});
	}
}
