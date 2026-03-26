#pragma once

#include "WindowMode.h"
#include "WindowProperties.h"

#include "WindowModule/WindowHandle.h"
#include "WindowModule/Config.h"

#include <EventSystem/EventListener.h>

#include <RHIModule/Graphics/Swapchain.h>

#include <CoreUtilities/Pointers/IntRef.h>
#include <CoreUtilities/Pointers/RawPtr.h>
#include <CoreUtilities/Pointers/Unique.h>

#include <functional>

#include <glm/glm.hpp>

struct GLFWwindow;
struct GLFWcursor;

namespace Volt
{
	class Event;

	enum class CursorType
	{
		Arrow = 0,
		TextInput,
		ResizeNS,
		ResizeEW,
		ResizeNESW,
		ResizeNWSE,
		ResizeAll,
		NotAllowed,
		Hand,
		Num
	};

	class WINDOWMODULE_API Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		Window(WindowHandle windowHandle,const WindowProperties& aProperties, bool forceSDR);
		~Window();

		void Shutdown();

		void Invalidate();
		void Release();

		void BeginFrame();
		void Render(float timestep);
		void Present();

		void Resize(uint32_t aWidth, uint32_t aHeight);
		void SetViewportSize(uint32_t width, uint32_t height);
		void SetPosition(int32_t x, int32_t y);
		void SetWindowMode(WindowMode aWindowMode, bool first = false);
		void SetVsync(bool aState);
		void SetTitle(const String& title);

		void SetIcon(const Filesystem::Path& path);
		void EnableMousePassthrough(bool state);

		void Maximize() const;
		void Minimize() const;
		void Restore() const;
		void Show() const;
		void Hide() const;
		void ShowCursor(bool state);
		void Focus() const;

		bool IsFocused() const;
		bool IsHovered() const;
		bool IsMaximized() const;
		bool IsMinimized() const;
		bool IsCursorEnabled() const;

		void ReplaceCursor(CursorType cursorType, const Filesystem::Path& path);
		void SetCursor(CursorType cursorType);

		void SetOpacity(float opacity) const;
		StringView GetClipboard() const;
		void SetClipboard(StringView string);

		const std::pair<float, float> GetPosition() const;
		const std::pair<int32_t, int32_t> GetFramebufferSize() const;

		const std::pair<float, float> GetCursorPos() const;

		const float GetOpacity() const;
		const float GetTime() const;

		const String& GetTitle();

		inline const uint32_t GetWidth() const { return m_data.width; }
		inline const uint32_t GetHeight() const { return m_data.height; }
		inline const uint32_t GetViewportWidth() const { return m_viewportWidth; }
		inline const uint32_t GetViewportHeight() const { return m_viewportHeight; }

		inline WindowHandle GetHandle() const { return m_windowHandle; }
		inline const bool IsVSync() const { return m_data.vsync; }
		inline const WindowMode GetWindowMode() const { return m_data.windowMode; }
		inline GLFWwindow* GetNativeWindow() const { return m_window; }
		inline void* GetHWND() const { return m_HWIND; }
		inline const auto& GetCursors() const { return m_cursors; }

		inline const RHI::Swapchain& GetSwapchain() const { return *m_swapchain; }
		inline const RawPtr<RHI::Swapchain> GetSwapchainPtr() const { return m_swapchain; }

		static Unique<Window> Create(WindowHandle handle, const WindowProperties& aProperties, bool forceSDR);

	private:
		class WindowEventListener : public EventListener
		{
		public:
			WindowEventListener(GLFWwindow* glfwWindow);
			~WindowEventListener() override = default;

		private:
			GLFWwindow* m_window = nullptr;
		};

		void CreateDefaultCursors();

		const WindowHandle m_windowHandle;

		GLFWwindow* m_window = nullptr;
		void* m_HWIND = nullptr;
		bool m_hasBeenInitialized = false;
		bool m_isFullscreen = false;

		struct WindowData
		{
			String title;
			Filesystem::Path iconPath;
			Filesystem::Path cursorPath;
			uint32_t width;
			uint32_t height;
			bool vsync;
			WindowMode windowMode;
			bool forceSDR;

		} m_data;

		IntRef<RHI::Swapchain> m_swapchain;

		glm::uvec2 m_startPosition = 0;
		glm::uvec2 m_startSize = 0;

		uint32_t m_viewportWidth = 0;
		uint32_t m_viewportHeight = 0;

		Unique<WindowEventListener> m_eventListener;
		WindowProperties m_properties;
		Array<GLFWcursor*, static_cast<size_t>(CursorType::Num)> m_cursors;

		bool m_frameHasStarted = false;
		bool m_shouldSkipDispatchResizeEvent = false;
	};
}
