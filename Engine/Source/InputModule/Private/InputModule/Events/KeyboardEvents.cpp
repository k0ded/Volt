#include "inputpch.h"
#include "Events/KeyboardEvents.h"

namespace Volt
{
	String KeyPressedEvent::ToString() const
	{
		return FormatString("KeyPressedEvent: {} ({} repeats)", Volt::ToString(m_keyCode), m_repeatCount);
	}

	String KeyReleasedEvent::ToString() const
	{
		return FormatString("KeyReleasedEvent: {}", Volt::ToString(m_keyCode));
	}

	String KeyTypedEvent::ToString() const
	{
		return FormatString("KeyTypedEvent: {}", m_character);
	}
}
