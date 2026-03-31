#include "windowpch.h"

#include "WindowModule/Events/WindowEvents_New.h"
#include "WindowModule/Window_New.h"

namespace Volt
{
	String WindowCloseEvent_New::ToString() const
	{
		return "WindowCloseEvent";
	}

	String WindowResizeEvent_New::ToString() const
	{
		return FormatString("WindowResizeEvent: {}, {}", m_width, m_height);
	}

	String WindowRepaintEvent::ToString() const
	{
		return "WindowRepaintEvent";
	}

	String WindowRenderEvent_New::ToString() const
	{
		return FormatString("WindowRenderEvent: {}", m_window.GetTitle());
	}
}
