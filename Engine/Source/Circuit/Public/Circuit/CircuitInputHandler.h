#pragma once
#include "Circuit/Config.h"

#include <CoreUtilities/Containers/Vector.h>

#include <CoreUtilities/Delegates/DelegateDeclarationHelpers.h>
#include <EventSystem/EventListener.h>

#include <InputModule/InputCodes.h>

namespace Volt
{
	class MouseMovedEvent;
	class MouseButtonPressedEvent;
	class MouseButtonReleasedEvent;
	class WindowTitlebarHittestEvent;
}
namespace Circuit
{
	class CircuitWindow;
	class Widget;
	class CIRCUIT_API CircuitInputHandler : public Volt::EventListener
	{
	public:
		void Init();
	private:
		void RegisterEventListeners();

		Vector<Ref<Widget>> GetWidgetsUnderCursor();
		Ref<CircuitWindow> GetHoveredWindow();
		Ref<Widget> GetHoveredWidget();

		void MouseMove(const glm::vec2 mouseScreenPos);

		bool OnMouseMoved(Volt::MouseMovedEvent& e);
		bool OnWindowTitlebarHittest(Volt::WindowTitlebarHittestEvent& e);
		bool OnMouseButtonPressed(Volt::MouseButtonPressedEvent& e);
		bool OnMouseButtonReleased(Volt::MouseButtonReleasedEvent& e);

		glm::vec2 m_mousePos;

		//dragging
		Weak<Widget> m_draggingWidget;
		glm::vec2 m_startDragMousePos;
		Volt::InputCode m_dragMouseButton;
		bool m_isDraggingWidget;
		static constexpr int32_t MIN_DRAG_DELTA_THRESHOLD = 5; // in pixels

		Weak<Widget> m_prevHoveredWidget;

	};
}
