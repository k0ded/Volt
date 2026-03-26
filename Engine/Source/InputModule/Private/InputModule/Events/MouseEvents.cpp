#include "inputpch.h"

#include "Events/MouseEvents.h"

namespace Volt
{
	String MouseMovedEvent::ToString() const
	{
		return FormatString("MouseMovedEvent: {}, {}", m_mouseX, m_mouseY);
	}

	String MouseButtonPressedEvent::ToString() const
	{
		return FormatString("MouseButtonPressedEvent: {}", Volt::ToString(GetMouseButton()));
	}

	String MouseButtonReleasedEvent::ToString() const
	{
		return FormatString("MouseButtonReleasedEvent: {}", Volt::ToString(GetMouseButton()));
	}
}
