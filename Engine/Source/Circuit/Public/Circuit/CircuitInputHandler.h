#pragma once
#include "Circuit/Config.h"

#include <CoreUtilities/Containers/Vector.h>

#include <CoreUtilities/Delegates/DelegateDeclarationHelpers.h>
#include <EventSystem/EventListener.h>

namespace Volt
{
	class MouseMovedEvent;
	class MouseButtonPressedEvent;
	class MouseButtonReleasedEvent;
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


		bool OnMouseMoved(Volt::MouseMovedEvent& e);
		bool OnMouseButtonPressed(Volt::MouseButtonPressedEvent& e);
		bool OnMouseButtonReleased(Volt::MouseButtonReleasedEvent& e);

		glm::vec2 m_mousePos;

		Weak<Widget> m_prevHoveredWidget;
	};
}
