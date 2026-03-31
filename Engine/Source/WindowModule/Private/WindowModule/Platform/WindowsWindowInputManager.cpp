#include "windowpch.h"

#include "WindowModule/Platform/WindowsWindowInputManager.h"
#include "WindowModule/Window_New.h"

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	static InputCode GetInputCodeFromVK(WPARAM wParam, LPARAM lParam)
	{
		if ((wParam == VK_RETURN) && (HIWORD(lParam) & KF_EXTENDED))
		{
			return InputCode::Numpad_Enter;
		}

		switch (wParam)
		{
			case VK_TAB:			return InputCode::Tab;
			case VK_LEFT:			return InputCode::LeftArrow;
			case VK_RIGHT:			return InputCode::RightArrow;
			case VK_UP:				return InputCode::UpArrow;
			case VK_DOWN:			return InputCode::DownArrow;
			case VK_PRIOR:			return InputCode::PageUp;
			case VK_NEXT:			return InputCode::PageDown;
			case VK_HOME:			return InputCode::Home;
			case VK_END:			return InputCode::End;
			case VK_INSERT:			return InputCode::Insert;
			case VK_DELETE:			return InputCode::Delete;
			case VK_BACK:			return InputCode::Backspace;
			case VK_SPACE:			return InputCode::Spacebar;
			case VK_RETURN:			return InputCode::Enter;
			case VK_OEM_COMMA:		return InputCode::Comma;
			case VK_OEM_PERIOD:		return InputCode::Period;
			case VK_CAPITAL:		return InputCode::CapsLock;
			case VK_SCROLL:			return InputCode::ScrollLock;
			case VK_NUMLOCK:		return InputCode::NumLock;
			case VK_SNAPSHOT:		return InputCode::PrintScreen;
			case VK_PAUSE:			return InputCode::Pause;
			case VK_NUMPAD0:		return InputCode::Numpad_0;
			case VK_NUMPAD1:		return InputCode::Numpad_1;
			case VK_NUMPAD2:		return InputCode::Numpad_2;
			case VK_NUMPAD3:		return InputCode::Numpad_3;
			case VK_NUMPAD4:		return InputCode::Numpad_4;
			case VK_NUMPAD5:		return InputCode::Numpad_5;
			case VK_NUMPAD6:		return InputCode::Numpad_6;
			case VK_NUMPAD7:		return InputCode::Numpad_7;
			case VK_NUMPAD8:		return InputCode::Numpad_8;
			case VK_NUMPAD9:		return InputCode::Numpad_9;
			case VK_DECIMAL:		return InputCode::Numpad_Decimal;
			case VK_DIVIDE:			return InputCode::Numpad_Divide;
			case VK_MULTIPLY:		return InputCode::Numpad_Multiply;
			case VK_SUBTRACT:		return InputCode::Numpad_Subtract;
			case VK_ADD:			return InputCode::Numpad_Add;
			case VK_LSHIFT:			return InputCode::LeftShift;
			case VK_LCONTROL:		return InputCode::LeftControl;
			case VK_LMENU:			return InputCode::LeftAlt;
			case VK_RSHIFT:			return InputCode::RightShift;
			case VK_RCONTROL:		return InputCode::RightControl;
			case VK_RMENU:			return InputCode::RightAlt;
			case VK_RWIN:			return InputCode::RightSuper;
			case VK_APPS:			return InputCode::Menu;

			case '0':				return InputCode::Key_0;
			case '1':				return InputCode::Key_1;
			case '2':				return InputCode::Key_2;
			case '3':				return InputCode::Key_3;
			case '4':				return InputCode::Key_4;
			case '5':				return InputCode::Key_5;
			case '6':				return InputCode::Key_6;
			case '7':				return InputCode::Key_7;
			case '8':				return InputCode::Key_8;
			case '9':				return InputCode::Key_9;

			case 'A':				return InputCode::A;
			case 'B':				return InputCode::B;
			case 'C':				return InputCode::C;
			case 'D':				return InputCode::D;
			case 'E':				return InputCode::E;
			case 'F':				return InputCode::F;
			case 'G':				return InputCode::G;
			case 'H':				return InputCode::H;
			case 'I':				return InputCode::I;
			case 'J':				return InputCode::J;
			case 'K':				return InputCode::K;
			case 'L':				return InputCode::L;
			case 'M':				return InputCode::M;
			case 'N':				return InputCode::N;
			case 'O':				return InputCode::O;
			case 'P':				return InputCode::P;
			case 'Q':				return InputCode::Q;
			case 'R':				return InputCode::R;
			case 'S':				return InputCode::S;
			case 'T':				return InputCode::T;
			case 'U':				return InputCode::U;
			case 'V':				return InputCode::V;
			case 'W':				return InputCode::W;
			case 'X':				return InputCode::X;
			case 'Y':				return InputCode::Y;
			case 'Z':				return InputCode::Z;

			case VK_F1:				return InputCode::F1;
			case VK_F2:				return InputCode::F2;
			case VK_F3:				return InputCode::F3;
			case VK_F4:				return InputCode::F4;
			case VK_F5:				return InputCode::F5;
			case VK_F6:				return InputCode::F6;
			case VK_F7:				return InputCode::F7;
			case VK_F8:				return InputCode::F8;
			case VK_F9:				return InputCode::F9;
			case VK_F10:			return InputCode::F10;
			case VK_F11:			return InputCode::F11;
			case VK_F12:			return InputCode::F12;
			case VK_F13:			return InputCode::F13;
			case VK_F14:			return InputCode::F14;
			case VK_F15:			return InputCode::F15;
			case VK_F16:			return InputCode::F16;
			case VK_F17:			return InputCode::F17;
			case VK_F18:			return InputCode::F18;
			case VK_F19:			return InputCode::F19;
			case VK_F20:			return InputCode::F20;
			case VK_F21:			return InputCode::F21;
			case VK_F22:			return InputCode::F22;
			case VK_F23:			return InputCode::F23;
			case VK_F24:			return InputCode::F24;

			//case VK_BROWSER_BACK:	return InputCode::AppBack;
			//case VK_BROWSER_FORWARD: return InputCode::AppForward;
		}

		const int32_t scancode = static_cast<int32_t>(LOBYTE(HIWORD(lParam)));

		switch (scancode)
		{
			case 41: return InputCode::GraveAccent; // VK_OEM_8 in EN-UK, VK_OEM_3 in EN-US, VK_OEM_7 in FR, VK_OEM_5 in DE, etc.
			case 12: return InputCode::Minus;
			case 13: return InputCode::Equal;
			case 26: return InputCode::LeftBracket;
			case 27: return InputCode::RightBracket;
			//case 86: return InputCode::Oem102;
			case 43: return InputCode::Backslash;
			case 39: return InputCode::Semicolon;
			case 40: return InputCode::Apostrophe;
			case 51: return InputCode::Comma;
			case 52: return InputCode::Period;
			case 53: return InputCode::Slash;
		}

		return InputCode::Unknown;
	}

	WindowsWindowInputManager::WindowsWindowInputManager(Window_New* ownerWindow)
		: m_ownerWindow(ownerWindow)
	{
		UpdateKeyboardCodePage();
	}

	bool WindowsWindowInputManager::IsKeyPressed(InputCode keyCode) const
	{
		VT_ENSURE_MSG(std::to_underlying(keyCode) < std::to_underlying(InputCode::Mouse_LB), "keyCode is not a key code!");

		const uint32_t keyIndex = static_cast<uint32_t>(keyCode);
		return m_currentKeyStates.Test(keyIndex) && !m_prevKeyStates.Test(keyIndex);
	}
	
	bool WindowsWindowInputManager::IsKeyReleased(InputCode keyCode) const
	{
		VT_ENSURE_MSG(std::to_underlying(keyCode) < std::to_underlying(InputCode::Mouse_LB), "keyCode is not a key code!");

		const uint32_t keyIndex = static_cast<uint32_t>(keyCode);
		return !m_currentKeyStates.Test(keyIndex) && m_prevKeyStates.Test(keyIndex);
	}

	bool WindowsWindowInputManager::IsMouseButtonPressed(InputCode mouseButtonCode) const
	{
		VT_ENSURE_MSG(std::to_underlying(mouseButtonCode) >= std::to_underlying(InputCode::Mouse_LB) &&
			std::to_underlying(mouseButtonCode) < std::to_underlying(InputCode::Unknown), "mouseButtonCode is not a mouse button!");

		const uint32_t keyIndex = static_cast<uint32_t>(mouseButtonCode);
		return m_currentKeyStates.Test(keyIndex) && !m_prevKeyStates.Test(keyIndex);
	}

	bool WindowsWindowInputManager::IsMouseButtonReleased(InputCode mouseButtonCode) const
	{
		VT_ENSURE_MSG(std::to_underlying(mouseButtonCode) >= std::to_underlying(InputCode::Mouse_LB) &&
			std::to_underlying(mouseButtonCode) < std::to_underlying(InputCode::Unknown), "mouseButtonCode is not a mouse button!");

		const uint32_t keyIndex = static_cast<uint32_t>(mouseButtonCode);
		return !m_currentKeyStates.Test(keyIndex) && m_prevKeyStates.Test(keyIndex);
	}
	
	bool WindowsWindowInputManager::IsKeyDown(InputCode keyCode) const
	{
		VT_ENSURE_MSG(std::to_underlying(keyCode) < std::to_underlying(InputCode::Mouse_LB), "keyCode is not a key code!");

		const uint32_t keyIndex = static_cast<uint32_t>(keyCode);
		return m_currentKeyStates.Test(keyIndex) && m_prevKeyStates.Test(keyIndex);
	}

	bool WindowsWindowInputManager::IsKeyUp(InputCode keyCode) const
	{
		VT_ENSURE_MSG(std::to_underlying(keyCode) < std::to_underlying(InputCode::Mouse_LB), "keyCode is not a key code!");

		const uint32_t keyIndex = static_cast<uint32_t>(keyCode);
		return !m_currentKeyStates.Test(keyIndex) && !m_prevKeyStates.Test(keyIndex);
	}

	bool WindowsWindowInputManager::IsMouseButtonDown(InputCode mouseButtonCode) const
	{
		VT_ENSURE_MSG(std::to_underlying(mouseButtonCode) >= std::to_underlying(InputCode::Mouse_LB) &&
			std::to_underlying(mouseButtonCode) < std::to_underlying(InputCode::Unknown), "mouseButtonCode is not a mouse button!");

		const uint32_t keyIndex = static_cast<uint32_t>(mouseButtonCode);
		return m_currentKeyStates.Test(keyIndex) && m_prevKeyStates.Test(keyIndex);
	}

	bool WindowsWindowInputManager::IsMouseButtonUp(InputCode mouseButtonCode) const
	{
		VT_ENSURE_MSG(std::to_underlying(mouseButtonCode) >= std::to_underlying(InputCode::Mouse_LB) &&
			std::to_underlying(mouseButtonCode) < std::to_underlying(InputCode::Unknown), "mouseButtonCode is not a mouse button!");

		const uint32_t keyIndex = static_cast<uint32_t>(mouseButtonCode);
		return !m_currentKeyStates.Test(keyIndex) && !m_prevKeyStates.Test(keyIndex);
	}

	float WindowsWindowInputManager::GetMouseX() const
	{
		return m_mouseX;
	}

	float WindowsWindowInputManager::GetMouseY() const
	{
		return m_mouseY;
	}

	void WindowsWindowInputManager::EnableAutoRepeat()
	{
		m_isAutoRepeatEnabled = true;
	}

	void WindowsWindowInputManager::DisableAutoRepeat()
	{
		m_isAutoRepeatEnabled = false;
	}

	WindowsWindowInputManager::OnKeyEvent& WindowsWindowInputManager::GetOnKeyEvent()
	{
		return m_onKeyEvent;
	}

	WindowsWindowInputManager::OnMouseEvent& WindowsWindowInputManager::GetOnMouseEvent()
	{
		return m_onMouseEvent;
	}

	void WindowsWindowInputManager::ClearFrameState()
	{
		m_prevKeyStates = m_currentKeyStates;
		m_currentKeyStates.Reset();
	}

	void WindowsWindowInputManager::ClearState()
	{
		m_prevKeyStates.Reset();
		m_currentKeyStates.Reset();
	}

	void WindowsWindowInputManager::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		VT_PROFILE_FUNCTION();

		switch (msg)
		{
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
			{
				const bool isRepeatEvent = (lParam & (1u << 30u));
				if (!isRepeatEvent || m_isAutoRepeatEnabled)
				{
					OnKeyPressed(wParam, lParam);
				}

				break;
			}

			case WM_SYSKEYUP:
			case WM_KEYUP:
			{
				OnKeyReleased(wParam, lParam);
				break;
			}

			case WM_CHAR:
			{
				OnChar(hWnd, wParam);
				break;
			}

			case WM_INPUTLANGCHANGE:
			{
				UpdateKeyboardCodePage();
				break;
			}

			case WM_KILLFOCUS:
			{
				ClearState();
				break;
			}

			case WM_MOUSEMOVE:
			{
				POINTS pt = MAKEPOINTS(lParam);

				if (pt.x >= 0 && pt.x < m_ownerWindow->GetWidth() && pt.y >= 0 && pt.y < m_ownerWindow->GetHeight())
				{
					OnMouseMoved(pt.x, pt.y);
				
					if (!m_isMouseInWindow)
					{
						OnMouseEnter(pt.x, pt.y);
					}
				}
				// Only generate once.
				else if (m_isMouseInWindow)
				{
					OnMouseLeave(pt.x, pt.y);
				}

				break;
			}

			case WM_LBUTTONDOWN:
			{
				OnMouseButtonPressed(InputCode::Mouse_LB);
				break;
			}

			case WM_RBUTTONDOWN:
			{
				OnMouseButtonPressed(InputCode::Mouse_RB);
				break;
			}

			case WM_MBUTTONDOWN:
			{
				OnMouseButtonPressed(InputCode::Mouse_MB);
				break;
			}


			case WM_LBUTTONUP:
			{
				OnMouseButtonReleased(InputCode::Mouse_LB);
				break;
			}

			case WM_RBUTTONUP:
			{
				OnMouseButtonReleased(InputCode::Mouse_RB);
				break;
			}

			case WM_MBUTTONUP:
			{
				OnMouseButtonReleased(InputCode::Mouse_MB);
				break;
			}

			case WM_MOUSEWHEEL:
			{
				OnMouseScroll(0.f, static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam) / static_cast<float>(WHEEL_DELTA)));
				break;
			}

			case WM_MOUSEHWHEEL:
			{
				OnMouseScroll(-static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam) / static_cast<float>(WHEEL_DELTA)), 0.f);
				break;
			}
		}
	}

	void WindowsWindowInputManager::OnKeyPressed(WPARAM wParam, LPARAM lParam)
	{
		const InputCode inputCode = GetInputCodeFromVK(wParam, lParam);
		m_currentKeyStates.SetBit(static_cast<uint32_t>(inputCode), true);

		m_onKeyEvent.Broadcast(KeyEvent(KeyEvent::Type::Press, inputCode));
	}
	
	void WindowsWindowInputManager::OnKeyReleased(WPARAM wParam, LPARAM lParam)
	{
		const InputCode inputCode = GetInputCodeFromVK(wParam, lParam);
		m_currentKeyStates.SetBit(static_cast<uint32_t>(inputCode), false);

		m_onKeyEvent.Broadcast(KeyEvent(KeyEvent::Type::Release, inputCode));
	}
	
	void WindowsWindowInputManager::OnChar(HWND hWnd, WPARAM wParam)
	{
		 if (::IsWindowUnicode(hWnd))
		 {
			 if (wParam > 0 && wParam < 0x10000)
			 {
				 ProcessUTF16Character(static_cast<wchar_t>(wParam));
			 }
		 }
		 else
		 {
			 wchar_t wideChar = 0;
			 ::MultiByteToWideChar(m_keyboardCodePage, MB_PRECOMPOSED, (char*)&wParam, 2, &wideChar, 1);
			 m_characterQueue.Enqueue(static_cast<uint32_t>(wideChar));
		 }
	}

	void WindowsWindowInputManager::OnMouseMoved(float x, float y)
	{
		m_mouseX = x;
		m_mouseY = y;

		m_onMouseEvent.Broadcast(MouseEvent(MouseEvent::Type::Move, InputCode::Unknown, x, y));
	}

	void WindowsWindowInputManager::OnMouseButtonPressed(InputCode mouseButtonCode)
	{
		m_currentKeyStates.SetBit(static_cast<uint32_t>(mouseButtonCode), true);
		m_onMouseEvent.Broadcast(MouseEvent(MouseEvent::Type::Press, mouseButtonCode, 0.f, 0.f));
	}

	void WindowsWindowInputManager::OnMouseButtonReleased(InputCode mouseButtonCode)
	{
		m_currentKeyStates.SetBit(static_cast<uint32_t>(mouseButtonCode), false);
		m_onMouseEvent.Broadcast(MouseEvent(MouseEvent::Type::Release, mouseButtonCode, 0.f, 0.f));
	}

	void WindowsWindowInputManager::OnMouseScroll(float x, float y)
	{
		m_onMouseEvent.Broadcast(MouseEvent(MouseEvent::Type::Scroll, InputCode::Unknown, x, y));
	}

	void WindowsWindowInputManager::OnMouseEnter(float x, float y)
	{
		m_isMouseInWindow = true;
		m_onMouseEvent.Broadcast(MouseEvent(MouseEvent::Type::Enter, InputCode::Unknown, x, y));
	}

	void WindowsWindowInputManager::OnMouseLeave(float x, float y)
	{
		m_isMouseInWindow = false;
		m_onMouseEvent.Broadcast(MouseEvent(MouseEvent::Type::Leave, InputCode::Unknown, x, y));
	}

	void WindowsWindowInputManager::ProcessUTF16Character(wchar_t character)
	{
		if (character == 0 && m_inputQueueSurrogate == 0)
		{
			return;
		}

		if ((character & 0xFC00) == 0xD800)
		{
			if (m_inputQueueSurrogate != 0)
			{
				// Invalid character
			}
			m_inputQueueSurrogate = character;
			return;
		}

		wchar_t codepoint = character;
		if (m_inputQueueSurrogate != 0)
		{
			if ((character & 0xFC00) != 0xDC00)
			{
				// Invalid character
			}
			else
			{
				// Invalid character for wchar_t
			}
	
			m_inputQueueSurrogate = 0;
		}

		m_characterQueue.Enqueue(static_cast<uint32_t>(codepoint));
	}

	void WindowsWindowInputManager::UpdateKeyboardCodePage()
	{
		HKL keyboardLayout = ::GetKeyboardLayout(0);
		LCID keyboardLCID = MAKELCID(HIWORD(keyboardLayout), SORT_DEFAULT);

		if (::GetLocaleInfoA(keyboardLCID, (LOCALE_RETURN_NUMBER | LOCALE_IDEFAULTANSICODEPAGE), (LPSTR)&m_keyboardCodePage, sizeof(m_keyboardCodePage)) == 0)
		{
			m_keyboardCodePage = CP_ACP;
		}
	}
}
