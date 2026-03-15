#include "circuitpch.h"
#include "CircuitInputHandler.h"

#include "Circuit/Widgets/Widget.h"

#include "Circuit/CircuitManager.h"
#include "Circuit/Window/CircuitWindow.h"
#include "Circuit/Reply.h"
#include "Circuit/WidgetInteractionData.h"

#include <InputModule/Events/MouseEvents.h>

#include <CoreUtilities/Containers/Queue.h>
#include <CoreUtilities/Delegates/Delegate.h>

#include <WindowModule/Window.h>

#include <LogModule/Log.h>

namespace Circuit
{
	void Circuit::CircuitInputHandler::Init()
	{
		m_isDraggingWidget = false;
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
		Vector<Ref<Widget>> widgetsToCheck;

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

		if (windowWidget->GetAllotedScreenArea().IsPointInside(m_mousePos))
		{
			widgetsToCheck.push_back(windowWidget);
		}

		//keep adding widgets that we find are under the cursor
		for (int i = 0; i < widgetsToCheck.size(); i++)
		{
			Ref<Widget> checkingWidget = widgetsToCheck[i];
			if (!checkingWidget)
			{
				continue;
			}

			if (checkingWidget->GetBounds().IsPointInside(m_mousePos))
			{
				widgetsUnderCursor.push_back(checkingWidget);
			}

			if (!checkingWidget->HasChildren())
			{
				continue;
			}

			for (Ref<Widget> child : checkingWidget->GetChildren())
			{
				if (child->GetAllotedScreenArea().IsPointInside(m_mousePos))
				{
					widgetsToCheck.push_back(child);
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

		VT_LOG(Info, "Num Widgets Under Cursor: {}", widgetsUnderCursor.size());

		//the bottommost widget will be first in the list, traverse it backwards
		for (int32_t i = static_cast<int32_t>(widgetsUnderCursor.size()) - 1; i >= 0; i--)
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
		m_mousePos = { e.GetWindow().GetPosition().first + e.GetX(), e.GetWindow().GetPosition().second + e.GetY() };

		Ref<Widget> hoveredWidget = GetHoveredWidget();

		if (m_draggingWidget)
		{
			const glm::vec2 dragDelta =  m_startDragMousePos - m_mousePos;
			if (!m_isDraggingWidget &&
				glm::length(dragDelta) >= MIN_DRAG_DELTA_THRESHOLD )
			{
				WidgetInteractionData startDragInteractionData;
				startDragInteractionData.mouseButton = m_dragMouseButton;
				startDragInteractionData.mousePos = m_startDragMousePos;

				m_draggingWidget->OnBeginDrag(startDragInteractionData);

				m_isDraggingWidget = true;
			}

			if (m_isDraggingWidget)
			{
				WidgetInteractionData interactionData;
				interactionData.mouseButton = m_dragMouseButton;
				interactionData.mousePos = m_mousePos;
				interactionData.mouseDragDelta = dragDelta;

				m_draggingWidget->OnDrag(interactionData);
			}
		}

		if (hoveredWidget != m_prevHoveredWidget)
		{
			WidgetInteractionData interactionData;
			interactionData.mouseButton = Volt::InputCode::Unknown;
			interactionData.mousePos = m_mousePos;

			if (m_prevHoveredWidget)
			{
				m_prevHoveredWidget->OnEndHover(interactionData);
			}

			m_prevHoveredWidget = hoveredWidget;

			if (m_prevHoveredWidget)
			{
				m_prevHoveredWidget->OnBeginHover(interactionData);
			}
		}

		return false;
	}

	bool Circuit::CircuitInputHandler::OnMouseButtonPressed(Volt::MouseButtonPressedEvent& e)
	{
		if (m_prevHoveredWidget)
		{
			WidgetInteractionData interactionData;
			interactionData.mouseButton = e.GetMouseButton();
			interactionData.mousePos = m_mousePos;

			m_prevHoveredWidget->OnPressed(interactionData);

			m_draggingWidget = m_prevHoveredWidget;
			m_dragMouseButton = e.GetMouseButton();
			m_startDragMousePos = m_mousePos;
		}
		return false;
	}

	bool Circuit::CircuitInputHandler::OnMouseButtonReleased(Volt::MouseButtonReleasedEvent& e)
	{
		if (e.GetMouseButton() == m_dragMouseButton && m_draggingWidget)
		{
			if (m_isDraggingWidget)
			{
				WidgetInteractionData startDragInteractionData;
				startDragInteractionData.mouseButton = m_dragMouseButton;
				startDragInteractionData.mousePos = m_mousePos;

				m_draggingWidget->OnEndDrag(startDragInteractionData);

				m_isDraggingWidget = false;
			}

			m_draggingWidget.Reset();
		}

		if (m_prevHoveredWidget)
		{
			WidgetInteractionData interactionData;
			interactionData.mouseButton = e.GetMouseButton();
			interactionData.mousePos = m_mousePos;

			m_prevHoveredWidget->OnReleased(interactionData);
		}
		return false;
	}
}
