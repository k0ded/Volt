#pragma once
#include "Circuit/Config.h"

#include <EventSystem/EventListener.h>

#include <InputModule/InputCodes.h>

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Delegates/DelegateDeclarationHelpers.h>
#include <CoreUtilities/Pointers/Weak.h>

#include <WindowModule/WindowHandle.h>

#include <unordered_set>
#include <chrono>

namespace Volt
{
	class WindowInputManager;
	class Window_New;

	class KeyEvent;
	class MouseEvent;
}
namespace Circuit
{
	class CircuitWindow;
	class Widget;
	class CIRCUIT_API CircuitInputHandler
	{
	public:
		void Init();

		void RegisterWindowInput(Volt::Window_New& window);
		void DeregisterWindowInput(Volt::Window_New& window);

		Vector<Ref<Widget>> GetWidgetsUnderCursor();
		Ref<CircuitWindow> GetHoveredWindow();
		Ref<Widget> GetHoveredWidget();
	private:


		void MouseMove(const glm::vec2 mouseScreenPos);

		void OnKeyEvent(const Volt::KeyEvent& keyEvent, Volt::WindowHandle windowHandle);
		void OnMouseEvent(const Volt::MouseEvent& mouseEvent, Volt::WindowHandle windowHandle);

		void OnMousePress(const Volt::MouseEvent& mouseEvent, Volt::WindowHandle windowHandle);
		void OnMouseRelease(const Volt::MouseEvent& mouseEvent, Volt::WindowHandle windowHandle);
		void OnMouseMove(const Volt::MouseEvent& mouseEvent, Volt::WindowHandle windowHandle);
		void OnMouseScroll(const Volt::MouseEvent& mouseEvent, Volt::WindowHandle windowHandle);
		void OnMouseLeaveWindow(const Volt::MouseEvent& mouseEvent, Volt::WindowHandle windowHandle);
		void OnMouseEnterWindow(const Volt::MouseEvent& mouseEvent, Volt::WindowHandle windowHandle);

		glm::vec2 GetMouseScreenPosFromEvent(const Volt::MouseEvent& mouseEvent, Volt::WindowHandle windowHandle) const;

		glm::vec2 m_mousePos;

		//dragging
		Weak<Widget> m_draggingWidget;
		glm::vec2 m_startDragMousePos;
		Volt::InputCode m_dragMouseButton;
		bool m_isDraggingWidget;
		static constexpr int32_t MIN_DRAG_DELTA_THRESHOLD = 5; // in pixels

		//double-click
		Volt::InputCode m_lastClickButton;
		Weak<Widget> m_lastClickWidget;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_lastClickedTime;


		Weak<Widget> m_prevHoveredWidget;




		Map<Volt::WindowHandle, Volt::DelegateHandle> m_registeredOnKeyEventWindows;
		Map<Volt::WindowHandle, Volt::DelegateHandle> m_registeredOnMouseEventWindows;
	};
}
