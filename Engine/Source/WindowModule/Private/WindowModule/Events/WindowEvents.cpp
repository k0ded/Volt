#include "windowpch.h"

#include "Events/WindowEvents.h"

namespace Volt
{
	String Volt::WindowResizeEvent::ToString() const
	{
		return FormatString("WindowResizeEvent: {}, {}", m_width, m_height);
	}
}
