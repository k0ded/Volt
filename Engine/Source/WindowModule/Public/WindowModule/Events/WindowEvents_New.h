#pragma once

#include "WindowModule/Config.h"

#include <EventSystem/Event.h>

namespace Volt
{
	class Window_New;

	class WindowEvent_New : public Event
	{
	public:
		VT_NODISCARD VT_INLINE Window_New& GetWindow() { return m_window; }

	protected:
		WindowEvent_New(Window_New& window)
			: m_window(window)
		{}

		Window_New& m_window;
	};

	class WindowCloseEvent_New : public WindowEvent_New
	{
	public:
		WindowCloseEvent_New(Window_New& window)
			: WindowEvent_New(window)
		{}

		WINDOWMODULE_API String ToString() const override;

		EVENT_CLASS(WindowCloseEvent_New, "{8688C5B7-E3DA-412C-B188-56BDD8F5354F}"_guid);
	};

	class WindowResizeEvent_New : public WindowEvent_New
	{
	public:
		WindowResizeEvent_New(Window_New& window, uint32_t width, uint32_t height)
			: WindowEvent_New(window), m_width(width), m_height(height)
		{}

		VT_INLINE uint32_t GetWidth() const { return m_width; }
		VT_INLINE uint32_t GetHeight() const { return m_height; }

		WINDOWMODULE_API String ToString() const override;

		EVENT_CLASS(WindowResizeEvent_New, "{F0BBC552-3ED0-456D-AC8A-EDC7F1B289F8}"_guid);

	private:
		uint32_t m_width;
		uint32_t m_height;
	};

	/*
		Dispatched when a window requires repaint and won't run the normal
		code path. Such as when resizing.
	*/
	class WindowRepaintEvent : public WindowEvent_New
	{
	public:
		WindowRepaintEvent(Window_New& window)
			: WindowEvent_New(window)
		{}

		WINDOWMODULE_API String ToString() const override;

		EVENT_CLASS(WindowResizeEvent_New, "{ED307C76-0ABE-4D48-A84F-6429FD607031}"_guid);
	};

	/*
		Dispatched when a window is ready to have it's contents rendered. I.e, we can render
		to the swapchain.
	*/
	class WindowRenderEvent_New : public WindowEvent_New
	{
	public:
		WindowRenderEvent_New(Window_New& window)
			: WindowEvent_New(window)
		{}

		WINDOWMODULE_API String ToString() const override;

		EVENT_CLASS(WindowRenderEvent_New, "{EA208202-6D7E-4073-882C-73B3FBE45650}"_guid);
	};
}
