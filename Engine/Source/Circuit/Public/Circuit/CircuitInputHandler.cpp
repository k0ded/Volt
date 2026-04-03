#include "circuitpch.h"
#include "CircuitInputHandler.h"

#include "Circuit/Widgets/Widget.h"
#include "Circuit/Widgets/WindowWidget.h"

#include "Circuit/CircuitManager.h"
#include "Circuit/Window/CircuitWindow.h"
#include "Circuit/Reply.h"
#include "Circuit/WidgetInteractionData.h"

#include <CoreUtilities/Containers/Queue.h>
#include <CoreUtilities/Delegates/Delegate.h>

#include <WindowModule/Window.h>
#include <WindowModule/Events/WindowEvents.h>
#include <WindowModule/Window_New.h>
#include <WindowModule/WindowInputManager.h>
#include <WindowModule/WindowManager_New.h>
#include <WindowModule/WindowInputEvents.h>

#include <LogModule/Log.h>


namespace Circuit
{
	void Circuit::CircuitInputHandler::Init()
	{
		m_isDraggingWidget = false;
	}

	void CircuitInputHandler::RegisterWindowInput(Volt::Window_New& window)
	{
		Volt::DelegateHandle keyEventHandle = window.GetInputManager().GetOnKeyEvent().AddRaw(this, &CircuitInputHandler::OnKeyEvent, window.GetHandle());
		m_registeredOnKeyEventWindows.insert({ window.GetHandle(), keyEventHandle });

		Volt::DelegateHandle mouseEventHandle = window.GetInputManager().GetOnMouseEvent().AddRaw(this, &CircuitInputHandler::OnMouseEvent, window.GetHandle());
		m_registeredOnMouseEventWindows.insert({ window.GetHandle(), mouseEventHandle });
	}

	void CircuitInputHandler::DeregisterWindowInput(Volt::Window_New& window)
	{
		VT_ENSURE(m_registeredOnKeyEventWindows.contains(window.GetHandle()));
		VT_ENSURE(m_registeredOnMouseEventWindows.contains(window.GetHandle()));

		window.GetInputManager().GetOnKeyEvent().Remove(m_registeredOnKeyEventWindows[window.GetHandle()]);
		m_registeredOnKeyEventWindows.erase(window.GetHandle());

		window.GetInputManager().GetOnMouseEvent().Remove(m_registeredOnMouseEventWindows[window.GetHandle()]);
		m_registeredOnMouseEventWindows.erase(window.GetHandle());
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

		Ref<WindowWidget> windowWidget = window->GetWidget().Lock();
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
		for (Weak<CircuitWindow> weakWindow : CircuitManager::Get().GetWindows())
		{
			if (weakWindow.IsExpired())
			{
				continue;
			}

			Ref<CircuitWindow> window = weakWindow.Lock();

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

	void CircuitInputHandler::MouseMove(const glm::vec2 mouseScreenPos)
	{
		m_mousePos = mouseScreenPos;

		Weak<Widget> hoveredWidget = GetHoveredWidget();

		if (!m_draggingWidget.IsExpired())
		{
			Ref<Widget> draggingWidget = m_draggingWidget.Lock();

			const glm::vec2 dragDelta = m_startDragMousePos - m_mousePos;
			if (!m_isDraggingWidget &&
				glm::length(dragDelta) >= MIN_DRAG_DELTA_THRESHOLD)
			{
				WidgetInteractionData startDragInteractionData;
				startDragInteractionData.mouseButton = m_dragMouseButton;
				startDragInteractionData.mousePos = m_startDragMousePos;

				draggingWidget->OnBeginDrag(startDragInteractionData);

				m_isDraggingWidget = true;
			}

			if (m_isDraggingWidget)
			{
				WidgetInteractionData interactionData;
				interactionData.mouseButton = m_dragMouseButton;
				interactionData.mousePos = m_mousePos;
				interactionData.mouseDragDelta = dragDelta;

				draggingWidget->OnDrag(interactionData);
			}
		}

		bool hoveredChanged = m_prevHoveredWidget.IsExpired() && !hoveredWidget.IsExpired();
		if (!hoveredChanged)
		{
			if (hoveredWidget.IsExpired())
			{
				hoveredChanged = true;
			}
			else
			{
				hoveredChanged = m_prevHoveredWidget.Lock() != hoveredWidget.Lock();
			}
		}
		if (hoveredChanged)
		{
			WidgetInteractionData interactionData;
			interactionData.mouseButton = Volt::InputCode::Unknown;
			interactionData.mousePos = m_mousePos;

			if (!m_prevHoveredWidget.IsExpired())
			{
				m_prevHoveredWidget.Lock()->OnEndHover(interactionData);
			}

			m_prevHoveredWidget = hoveredWidget;

			if (!m_prevHoveredWidget.IsExpired())
			{
				m_prevHoveredWidget.Lock()->OnBeginHover(interactionData);
			}
		}
	}

	void CircuitInputHandler::OnKeyEvent(const Volt::KeyEvent& keyEvent, Volt::WindowHandle windowHandle)
	{}

	void CircuitInputHandler::OnMouseEvent(const Volt::MouseEvent& mouseEvent, Volt::WindowHandle windowHandle)
	{
		switch (mouseEvent.GetType())
		{
			case Volt::MouseEvent::Type::Press:
				OnMousePress(mouseEvent, windowHandle);
				break;
			case Volt::MouseEvent::Type::Release:
				OnMouseRelease(mouseEvent, windowHandle);
				break;
			case Volt::MouseEvent::Type::Move:
				OnMouseMove(mouseEvent, windowHandle);
				break;
			case Volt::MouseEvent::Type::Scroll:
				OnMouseScroll(mouseEvent, windowHandle);
				break;
			case Volt::MouseEvent::Type::Leave:
				OnMouseLeaveWindow(mouseEvent, windowHandle);
				break;
			case Volt::MouseEvent::Type::Enter:
				OnMouseEnterWindow(mouseEvent, windowHandle);
				break;
		}
	}

	void CircuitInputHandler::OnMousePress(const Volt::MouseEvent& mouseEvent, Volt::WindowHandle windowHandle)
	{
		if (!m_prevHoveredWidget.IsExpired())
		{
			Ref<Widget> prevHoveredWidget = m_prevHoveredWidget.Lock();

			if (prevHoveredWidget)
			{
				WidgetInteractionData interactionData;
				interactionData.mouseButton = mouseEvent.GetMouseCode();
				interactionData.mousePos = m_mousePos;

				prevHoveredWidget->OnPressed(interactionData);

				m_draggingWidget = m_prevHoveredWidget;
				m_dragMouseButton = mouseEvent.GetMouseCode();
				m_startDragMousePos = m_mousePos;
			}
		}
	}

	void CircuitInputHandler::OnMouseRelease(const Volt::MouseEvent& mouseEvent, Volt::WindowHandle windowHandle)
	{
		//todo: hacky way to handle double click
			auto nowTime = std::chrono::high_resolution_clock::now();
		if (mouseEvent.GetMouseCode() == m_lastClickButton)
		{
			long long millisecondsSinceLastClick = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - m_lastClickedTime).count();
			if (millisecondsSinceLastClick < 300)
			{
				if (!m_prevHoveredWidget.IsExpired())
				{
					Ref<Widget> prevHoveredWidget = m_prevHoveredWidget.Lock();

					WidgetInteractionData interactionData;
					interactionData.mouseButton = mouseEvent.GetMouseCode();
					interactionData.mousePos = m_mousePos;

					prevHoveredWidget->OnDoubleClicked(interactionData);
				}
			}
		}
		m_lastClickButton = mouseEvent.GetMouseCode();
		m_lastClickedTime = nowTime;


		if (mouseEvent.GetMouseCode() == m_dragMouseButton && !m_draggingWidget.IsExpired())
		{
			Ref<Widget> draggingWidget = m_draggingWidget.Lock();

			if (m_isDraggingWidget)
			{
				WidgetInteractionData startDragInteractionData;
				startDragInteractionData.mouseButton = m_dragMouseButton;
				startDragInteractionData.mousePos = m_mousePos;

				draggingWidget->OnEndDrag(startDragInteractionData);

				m_isDraggingWidget = false;
			}

			m_draggingWidget.Reset();
		}


		if (!m_prevHoveredWidget.IsExpired())
		{
			Ref<Widget> prevHoveredWidget = m_prevHoveredWidget.Lock();

			WidgetInteractionData interactionData;
			interactionData.mouseButton = mouseEvent.GetMouseCode();
			interactionData.mousePos = m_mousePos;

			prevHoveredWidget->OnReleased(interactionData);
		}
	}

	void CircuitInputHandler::OnMouseMove(const Volt::MouseEvent& mouseEvent, Volt::WindowHandle windowHandle)
	{
		VT_LOG(Info, "MouseMoved: {}, {}", mouseEvent.GetX(), mouseEvent.GetY());


		MouseMove(GetMouseScreenPosFromEvent(mouseEvent, windowHandle));
	}

	void CircuitInputHandler::OnMouseScroll(const Volt::MouseEvent& mouseEvent, Volt::WindowHandle windowHandle)
	{}

	void CircuitInputHandler::OnMouseLeaveWindow(const Volt::MouseEvent& mouseEvent, Volt::WindowHandle windowHandle)
	{}

	void CircuitInputHandler::OnMouseEnterWindow(const Volt::MouseEvent& mouseEvent, Volt::WindowHandle windowHandle)
	{}

	glm::vec2 CircuitInputHandler::GetMouseScreenPosFromEvent(const Volt::MouseEvent& mouseEvent, Volt::WindowHandle windowHandle) const
	{
		const Volt::Window_New& window = Volt::WindowManager_New::Get().GetWindow(windowHandle);
		return glm::vec2(mouseEvent.GetX() + window.GetPositionX(), mouseEvent.GetY() + window.GetPositionY());
	}
}
