#pragma once

#include "InputModule/InputModuleConfig.h"

#include "InputModule/InputCodes.h"

#include <EventSystem/Event.h>

namespace Volt
{
	class Window;

	class INPUTMODULE_API KeyEvent : public Event
	{
	public:
		VT_NODISCARD VT_INLINE InputCode GetKeyCode() const { return m_keyCode; }
		VT_NODISCARD VT_INLINE int32_t GetScanCode() const { return m_scanCode; }
		VT_NODISCARD VT_INLINE Window& GetWindow() const { return m_window; }


		EVENT_CLASS(KeyEvent, "{F57124D9-554B-4A8F-9788-79641498BF1C}"_guid);
	protected:
		KeyEvent(Window& window, int32_t keyCode, int32_t scanCode)
			: m_window(window), m_scanCode(scanCode)
		{
			m_keyCode = GLFWKeyCodeToInputCode(keyCode);
		}

		InputCode m_keyCode;
		int32_t m_scanCode;
		Window& m_window;
	};

	class INPUTMODULE_API KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(Window& window, int32_t keyCode, int32_t scanCode, int32_t repeatCount)
			: KeyEvent(window, keyCode, scanCode), m_repeatCount(repeatCount)
		{
		}

		inline int GetRepeatCount() const { return m_repeatCount; }

		std::string ToString() const override;

		EVENT_CLASS(KeyPressedEvent, "{35E9A5ED-54B1-46F0-B5A3-3851BC78FC93}"_guid);
	private:
		int32_t m_repeatCount;
	};

	class INPUTMODULE_API KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(Window& window, int32_t keyCode, int32_t scanCode)
			: KeyEvent(window, keyCode, scanCode)
		{
		}

		std::string ToString() const override;

		EVENT_CLASS(KeyReleasedEvent, "{683AC3F1-9CEC-4BEA-9944-1740CD80E7A4}"_guid);
	};

	class INPUTMODULE_API KeyTypedEvent : public Event
	{
	public:
		KeyTypedEvent(Window& window, uint32_t character)
			: m_window(window), m_character(character)
		{
		}

		std::string ToString() const override;

		VT_NODISCARD VT_INLINE uint32_t GetCharacter() const { return m_character; }
		VT_NODISCARD VT_INLINE Window& GetWindow() const { return m_window; }

		EVENT_CLASS(KeyTypedEvent, "{E01DA431-6A5A-427D-8A99-2D1A3D4868AB}"_guid);
	
	private:
		Window& m_window;
		uint32_t m_character;
	};
}
