#pragma once

#include <InputModule/InputCodes.h>

namespace Volt
{
	class KeyEvent
	{
	public:
		enum class Type : uint8_t
		{
			Press,
			Release,
			Invalid
		};

		KeyEvent();
		KeyEvent(Type type, InputCode keyCode);

		VT_INLINE Type GetType() const { return m_eventType; }
		VT_INLINE InputCode GetKeyCode() const { return m_keyCode; }

	private:
		InputCode m_keyCode;
		Type m_eventType;
	};

	class MouseEvent
	{
	public:
		enum class Type : uint8_t
		{
			Press,
			Release,
			Move,
			Scroll,
			Leave,
			Enter,
			Invalid
		};

		MouseEvent();
		MouseEvent(Type type, InputCode mouseCode, float x, float y);

		VT_INLINE Type GetType() const { return m_eventType; }
		VT_INLINE InputCode GetMouseCode() const { return m_mouseCode; }
		VT_INLINE float GetX() const { return m_x; }
		VT_INLINE float GetY() const { return m_y; }

	private:
		InputCode m_mouseCode;
		float m_x;
		float m_y;
		Type m_eventType;
	};
}
