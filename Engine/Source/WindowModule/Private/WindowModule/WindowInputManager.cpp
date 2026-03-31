#include "windowpch.h"

#include "WindowModule/WindowInputManager.h"

namespace Volt
{
	WindowInputManager::KeyEvent::KeyEvent()
		: m_eventType(Type::Invalid),
		m_keyCode(InputCode::Unknown)
	{}

	WindowInputManager::KeyEvent::KeyEvent(Type type, InputCode keyCode)
		: m_eventType(type),
		m_keyCode(keyCode)
	{}

	WindowInputManager::MouseEvent::MouseEvent()
		: m_eventType(Type::Invalid),
		m_mouseCode(InputCode::Unknown),
		m_x(0.f),
		m_y(0.f)
	{}

	WindowInputManager::MouseEvent::MouseEvent(Type type, InputCode mouseCode, float x, float y)
		: m_eventType(type),
		m_mouseCode(mouseCode),
		m_x(x),
		m_y(y)
	{}
}
