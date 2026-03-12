#include "circuitpch.h"
#include "CircuitInputHandler.h"

#include "Circuit/Widgets/Widget.h"

#include "Circuit/CircuitManager.h"
#include "Circuit/Window/CircuitWindow.h"
#include "Circuit/Reply.h"

#include <InputModule/Events/MouseEvents.h>

#include <CoreUtilities/Containers/Queue.h>

#include <CoreUtilities/Delegates/Delegate.h>

namespace Circuit
{
	void Circuit::CircuitInputHandler::Init()
	{
		RegisterEventListeners();
	}

	void Circuit::CircuitInputHandler::RegisterEventListeners()
	{
		RegisterListener<Volt::MouseMovedEvent>(VT_BIND_EVENT_FN(CircuitInputHandler::OnMouseMoved));
		RegisterListener<Volt::MouseButtonPressedEvent>(VT_BIND_EVENT_FN(CircuitInputHandler::OnMouseButtonPressed));
		RegisterListener<Volt::MouseButtonReleasedEvent>(VT_BIND_EVENT_FN(CircuitInputHandler::OnMouseButtonReleased));
	}

	Vector<Ref<Widget>> CircuitInputHandler::GetWidgetsUnderCursor()
	{
		Vector<Ref<Widget>> widgetsUnderCursor;

		Ref<CircuitWindow> window = GetHoveredWindow();
		if (!window)
		{
			return Vector<Ref<Widget>>();
		}
		Ref<Widget> windowWidget = window->GetWidget();
		if (!windowWidget)
		{
			return Vector<Ref<Widget>>();
		}
		glm::vec2 mouseRelativeToWindow = m_mousePos - glm::vec2(window->GetPosition());

		if (windowWidget->GetBounds().IsPointInside(mouseRelativeToWindow))
		{
			widgetsUnderCursor.push_back(windowWidget);
		}

		//keep adding widgets that we find are under the cursor
		for (int i = 0; i < widgetsUnderCursor.size(); i++)
		{
			Ref<Widget> checkingWidget = widgetsUnderCursor[i];
			if (!checkingWidget)
			{
				continue;
			}

			if (!checkingWidget->HasChildren())
			{
				continue;
			}

			for (Ref<Widget> child : *checkingWidget->GetChildren())
			{
				if (child->GetBounds().IsPointInside(mouseRelativeToWindow))
				{
					widgetsUnderCursor.push_back(child);
				}
			}
		}
		return widgetsUnderCursor;
	}

	Ref<CircuitWindow> CircuitInputHandler::GetHoveredWindow()
	{
		for (Ref<CircuitWindow> window : CircuitManager::Get().GetWindows())
		{
			if (!window)
			{
				continue;
			}

			Volt::Rect windowRect(window->GetPosition(), window->GetSize());

			if (windowRect.IsPointInside(m_mousePos))
			{
				return window;
			}
		}

		return Ref<CircuitWindow>();
	}

	Ref<Widget> CircuitInputHandler::GetHoveredWidget()
	{
		Vector<Ref<Widget>> widgetsUnderCursor = GetWidgetsUnderCursor();
		for (int i = 0; i < widgetsUnderCursor.size(); i++)
		{
			Ref<Widget> widget = widgetsUnderCursor[i];
			if (widget->IsHittestInvisible())
			{
				continue;
			}
			return widget;
		}
		return Ref<Widget>();
	}

	bool Circuit::CircuitInputHandler::OnMouseMoved(Volt::MouseMovedEvent& e)
	{
		m_mousePos = { e.GetX(), e.GetY() };

		Ref<Widget> hoveredWidget = GetHoveredWidget();

		if (hoveredWidget != m_prevHoveredWidget)
		{
			if (m_prevHoveredWidget)
			{
				m_prevHoveredWidget->OnEndHover();
			}

			m_prevHoveredWidget = hoveredWidget;

			if (m_prevHoveredWidget)
			{
				m_prevHoveredWidget->OnBeginHover();
			}
		}

		return false;
	}

	bool Circuit::CircuitInputHandler::OnMouseButtonPressed(Volt::MouseButtonPressedEvent& e)
	{
		if (m_prevHoveredWidget)
		{
			m_prevHoveredWidget->OnPressed();
		}
		return false;
	}

	bool Circuit::CircuitInputHandler::OnMouseButtonReleased(Volt::MouseButtonReleasedEvent& e)
	{
		if (m_prevHoveredWidget)
		{
			m_prevHoveredWidget->OnReleased();
		}
		return false;
	}
}
