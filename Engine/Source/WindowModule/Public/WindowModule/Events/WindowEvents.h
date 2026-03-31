#pragma once
#include "WindowModule/Config.h"

#include <EventSystem/Event.h>

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Filesystem/Path.h>

namespace Volt
{
	class Window;
	class Monitor;

	class WINDOWMODULE_API WindowEvent : public Event
	{
	public:
		VT_NODISCARD VT_INLINE Window& GetWindow() { return m_window; }

	protected:
		WindowEvent(Window& window)
			: m_window(window)
		{ }

		Window& m_window;
	};

	class WINDOWMODULE_API WindowResizeEvent : public WindowEvent
	{
	public:
		WindowResizeEvent(Window& window, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
			: WindowEvent(window), m_width(width), m_height(height), m_x(x), m_y(y)
		{
		}

		WindowResizeEvent(Window& window, uint32_t width, uint32_t height)
			: WindowEvent(window), m_width(width), m_height(height), m_x(0), m_y(0)
		{
		}

		//Getting
		inline const uint32_t GetWidth() const { return m_width; }
		inline const uint32_t GetHeight() const { return m_height; }
		inline const uint32_t GetX() const { return m_x; }
		inline const uint32_t GetY() const { return m_y; }

		String ToString() const override;

		EVENT_CLASS(WindowResizeEvent, "{E045B613-AE06-41C9-8759-90B7E23AEED1}"_guid);

	private:
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_x;
		uint32_t m_y;
	};

	class WINDOWMODULE_API WindowCloseEvent : public WindowEvent
	{
	public:
		WindowCloseEvent(Window& window)
			: WindowEvent(window)
		{}

		EVENT_CLASS(WindowCloseEvent, "{01DFAC3F-FDB8-42E3-9903-22C0F4B9736F}"_guid);
	};

	class WINDOWMODULE_API WindowTitlebarHittestEvent : public WindowEvent
	{
	public:
		WindowTitlebarHittestEvent(Window& window, int x, int y, int& hit)
			: WindowEvent(window), m_x(x), m_y(y), m_hit(hit)
		{
		}

		inline const int32_t GetX() const { return m_x; }
		inline const int32_t GetY() const { return m_y; }

		inline void SetHit(bool hasHit) { m_hit = (int32_t)hasHit; }

		EVENT_CLASS(WindowTitlebarHittestEvent, "{5D627292-9BA3-4E8F-9FDE-AA53393B6384}"_guid);

	private:
		int m_x;
		int m_y;
		int& m_hit;
	};

	class WINDOWMODULE_API WindowRenderEvent : public WindowEvent
	{
	public:
		WindowRenderEvent(Window& window, float timestep)
			: WindowEvent(window), m_timestep(timestep)
		{ }

		VT_INLINE float GetTimestep() const { return m_timestep; }

		EVENT_CLASS(WindowRenderEvent, "{9775E1B6-2478-43FB-918D-C0B686AB328B}"_guid);
	private:
		float m_timestep;
	};

	class WINDOWMODULE_API WindowDragDropEvent : public WindowEvent
	{
	public:
		WindowDragDropEvent(Window& window, int32_t count, const char** paths)
			: WindowEvent(window)
		{
			for (int32_t i = 0; i < count; ++i)
			{
				m_paths.push_back(paths[i]);
			}
		}

		inline const Vector<Filesystem::Path>& GetPaths() const { return m_paths; }

		EVENT_CLASS(WindowDragDropEvent, "{FE76F668-D2AB-4EFF-8209-A8EFDC9EBDCF}"_guid);

	private:
		Vector<Filesystem::Path> m_paths;
	};

	class WINDOWMODULE_API WindowFocusChangedEvent : public WindowEvent
	{
	public:
		WindowFocusChangedEvent(Window& window, bool focused)
			: WindowEvent(window), m_focused(focused)
		{ }

		VT_NODISCARD VT_INLINE bool Focused() const { return m_focused; }

		EVENT_CLASS(WindowFocusChangedEvent, "{06CF0423-AB2C-4E7C-8BD2-5961FE3B996D}"_guid);

	private:
		bool m_focused;
	};

	class WINDOWMODULE_API WindowCursorEnteredEvent : public WindowEvent
	{
	public:
		WindowCursorEnteredEvent(Window& window, bool entered)
			: WindowEvent(window), m_entered(entered)
		{ }

		VT_NODISCARD VT_INLINE bool Entered() const { return m_entered; }

		EVENT_CLASS(WindowFocusChangedEvent, "{E742A4A1-7C58-4BEE-873C-43BFC066FBA2}"_guid);

	private:
		bool m_entered;
	};

	class WINDOWMODULE_API MonitorConnectedEvent : public Event
	{
	public:
		MonitorConnectedEvent(Monitor& monitor)
			: m_monitor(monitor)
		{}

		VT_NODISCARD VT_INLINE Monitor& GetMonitor() const { return m_monitor; }

		EVENT_CLASS(MonitorConnectedEvent, "{EF6241B8-B841-4F26-9CC9-D2271E1386EC}"_guid);
	private:
		Monitor& m_monitor;
	};

	class WINDOWMODULE_API MonitorDisconnectedEvent : public Event
	{
	public:
		MonitorDisconnectedEvent(Monitor& monitor)
			: m_monitor(monitor)
		{}

		VT_NODISCARD VT_INLINE Monitor& GetMonitor() const { return m_monitor; }

		EVENT_CLASS(MonitorDisconnectedEvent, "{EE88C8CB-4991-4685-9B89-D1F843049792}"_guid);
	private:
		Monitor& m_monitor;
	};
}
