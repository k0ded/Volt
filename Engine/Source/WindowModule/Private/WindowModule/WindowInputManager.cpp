#include "windowpch.h"

#include "WindowModule/WindowInputManager.h"

namespace Volt
{
	KeyEvent::KeyEvent()
		: m_eventType(Type::Invalid),
		m_keyCode(InputCode::Unknown)
	{}

	KeyEvent::KeyEvent(Type type, InputCode keyCode)
		: m_eventType(type),
		m_keyCode(keyCode)
	{}

	MouseEvent::MouseEvent()
		: m_eventType(Type::Invalid),
		m_mouseCode(InputCode::Unknown),
		m_x(0.f),
		m_y(0.f)
	{}

	MouseEvent::MouseEvent(Type type, InputCode mouseCode, float x, float y)
		: m_eventType(type),
		m_mouseCode(mouseCode),
		m_x(x),
		m_y(y)
	{}
}
