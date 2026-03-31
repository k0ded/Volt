#pragma once

#include <InputModule/InputCodes.h>

#include <CoreUtilities/Delegates/DelegateDeclarationHelpers.h>

namespace Volt
{
	class WindowInputManager
	{
	public:
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

		DECLARE_MULTICAST_DELEGATE_OneParam(OnKeyEvent, const KeyEvent&);
		DECLARE_MULTICAST_DELEGATE_OneParam(OnMouseEvent, const MouseEvent&);

		WindowInputManager() = default;
		virtual ~WindowInputManager() = default;

		WindowInputManager(WindowInputManager&&) = delete;
		WindowInputManager& operator=(WindowInputManager&&) = delete;
		WindowInputManager(const WindowInputManager&) = delete;
		WindowInputManager& operator=(const WindowInputManager&) = delete;

		virtual bool IsKeyPressed(InputCode keyCode) const = 0;
		virtual bool IsKeyReleased(InputCode keyCode) const = 0;
		virtual bool IsMouseButtonPressed(InputCode mouseButtonCode) const = 0;
		virtual bool IsMouseButtonReleased(InputCode mouseButtonCode) const = 0;

		virtual bool IsKeyDown(InputCode keyCode) const = 0;
		virtual bool IsKeyUp(InputCode keyCode) const = 0;
		virtual bool IsMouseButtonDown(InputCode mouseButtonCode) const = 0;
		virtual bool IsMouseButtonUp(InputCode mouseButtonCode) const = 0;

		virtual float GetMouseX() const = 0;
		virtual float GetMouseY() const = 0;

		virtual void EnableAutoRepeat() = 0;
		virtual void DisableAutoRepeat() = 0;

		virtual OnKeyEvent& GetOnKeyEvent() = 0;
		virtual OnMouseEvent& GetOnMouseEvent() = 0;
	};
}
