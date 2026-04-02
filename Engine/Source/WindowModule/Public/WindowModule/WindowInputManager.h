#pragma once

#include "WindowModule/WindowInputEvents.h"

#include <CoreUtilities/Delegates/DelegateDeclarationHelpers.h>

namespace Volt
{
	class WindowInputManager
	{
	public:
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
