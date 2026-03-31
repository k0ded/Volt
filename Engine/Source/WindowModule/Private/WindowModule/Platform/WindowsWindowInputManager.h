#pragma once

#include "WindowModule/WindowInputManager.h"

#include <CoreUtilities/Platform/Windows/VoltWindows.h>
#include <CoreUtilities/Containers/BitArray.h>
#include <CoreUtilities/Containers/Queue.h>

namespace Volt
{
	class Window_New;

	class WindowsWindowInputManager : public WindowInputManager
	{
	public:
		WindowsWindowInputManager(Window_New* ownerWindow);
		~WindowsWindowInputManager() override = default;

		bool IsKeyPressed(InputCode keyCode) const override;
		bool IsKeyReleased(InputCode keyCode) const override;
		bool IsMouseButtonPressed(InputCode mouseButtonCode) const override;
		bool IsMouseButtonReleased(InputCode mouseButtonCode) const override;

		bool IsKeyDown(InputCode keyCode) const override;
		bool IsKeyUp(InputCode keyCode) const override;
		bool IsMouseButtonDown(InputCode mouseButtonCode) const override;
		bool IsMouseButtonUp(InputCode mouseButtonCode) const override;

		float GetMouseX() const override;
		float GetMouseY() const override;

		void EnableAutoRepeat() override;
		void DisableAutoRepeat() override;

		OnKeyEvent& GetOnKeyEvent() override;
		OnMouseEvent& GetOnMouseEvent() override;

	private:
		friend class WindowsWindow;

		void ClearFrameState();
		void ClearState();

		void HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		void OnKeyPressed(WPARAM wParam, LPARAM lParam);
		void OnKeyReleased(WPARAM wParam, LPARAM lParam);
		void OnChar(HWND hWnd, WPARAM wParam);

		void OnMouseMoved(float x, float y);
		void OnMouseButtonPressed(InputCode mouseButtonCode);
		void OnMouseButtonReleased(InputCode mouseButtonCode);
		void OnMouseScroll(float x, float y);
		void OnMouseEnter(float x, float y);
		void OnMouseLeave(float x, float y);

		void ProcessUTF16Character(wchar_t character);
		void UpdateKeyboardCodePage();

		inline static constexpr uint32_t NumMaxKeys = static_cast<uint32_t>(InputCode::Unknown);

		OnKeyEvent m_onKeyEvent;
		OnMouseEvent m_onMouseEvent;

		BitArray<NumMaxKeys> m_prevKeyStates;
		BitArray<NumMaxKeys> m_currentKeyStates;
		Queue<uint32_t> m_characterQueue;

		Window_New* m_ownerWindow;

		float m_mouseX = 0.f;
		float m_mouseY = 0.f;

		uint32_t m_keyboardCodePage = 0;
		wchar_t m_inputQueueSurrogate = 0;
		bool m_isAutoRepeatEnabled = false;
		bool m_isMouseInWindow = false;
	};
}
