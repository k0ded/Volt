#include "windowpch.h"

#include "WindowModule/WindowInputManager.h"

namespace Volt
{
	KeyEvent::KeyEvent()
		: m_keyCode(InputCode::Unknown),
		m_eventType(Type::Invalid)
	{}

	KeyEvent::KeyEvent(Type type, InputCode keyCode)
		: m_keyCode(keyCode),
		m_eventType(type)
	{}

	MouseEvent::MouseEvent()
		: m_mouseCode(InputCode::Unknown),
		m_eventType(Type::Invalid),
		m_x(0.f),
		m_y(0.f)
	{}

	MouseEvent::MouseEvent(Type type, InputCode mouseCode, float x, float y)
		: m_mouseCode(mouseCode),
		m_eventType(type),
		m_x(x),
		m_y(y)
	{}
}
