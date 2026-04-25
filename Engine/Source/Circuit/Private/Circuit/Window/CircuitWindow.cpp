#include "circuitpch.h"
#include "Window/CircuitWindow.h"
#include "Circuit/CircuitPainter.h"

#include "Circuit/Widgets/Widget.h"
#include "Circuit/Widgets/WindowWidget.h"

#include "Circuit/Rendering/CircuitRenderer.h"

#include "Circuit/CircuitManager.h"
#include "Circuit/CircuitInputHandler.h"
#include "Circuit/ConsoleVars.h"
#include "Circuit/LogCategories.h"

#include <JobSystem/TaskGraph.h>

#include <WindowModule/WindowManager_New.h>
#include <WindowModule/Window_New.h>
#include <WindowModule/Events/WindowEvents.h>

#include <LogModule/Log.h>

namespace Circuit
{
	CircuitWindow::CircuitWindow(Volt::WindowHandle windowHandle)
		: m_windowHandle(windowHandle)
	{
		m_resourceTable = Volt::RHI::ResourceTable::Create();
		m_renderer = CreateRef<CircuitRenderer>(*this, m_resourceTable);

		{
			Volt::Window_New& window = Volt::WindowManager_New::Get().GetWindow(m_windowHandle);
			window.GetIsHoveringTitlebar().BindLambda([this](int32_t mouseX, int32_t mouseY)
			{
				return m_windowWidget->IsHoveringTitlebar();
			});
		}
	}

	CircuitWindow::~CircuitWindow()
	{
		VT_LOG(Info, "destruct CircuitWindow");
	}

	Volt::WindowHandle CircuitWindow::GetWindowHandle() const
	{
		return m_windowHandle;
	}

	CIRCUIT_API glm::i32vec2 CircuitWindow::GetPosition() const
	{
		Volt::Window_New& window = Volt::WindowManager_New::Get().GetWindow(m_windowHandle);
		return { window.GetPositionX(), window.GetPositionY() };
	}

	glm::u32vec2 CircuitWindow::GetSize() const
	{
		Volt::Window_New& window = Volt::WindowManager_New::Get().GetWindow(m_windowHandle);
		return { window.GetWidth(), window.GetHeight() };
	}
	void CircuitWindow::Resize(const glm::vec2& size)
	{
		m_windowSize = size;
	}

	ArrayView<CircuitDrawCommand> CircuitWindow::GetDrawCommands()
	{
		DoPaint();
		return m_drawCommands;
	}

	CIRCUIT_API void CircuitWindow::DoPaint()
	{
		VT_PROFILE_FUNCTION();

		const Volt::Rect windowScreenBounds = Volt::Rect(static_cast<float>(GetPosition().x), static_cast<float>(GetPosition().y), static_cast<float>(GetSize().x), static_cast<float>(GetSize().y));
		const glm::vec2 windowOrigin = windowScreenBounds.GetPosition();

		m_drawCommands.clear();

		if (!m_windowWidget)
		{
			return;
		}
		const bool logPaint = s_cvarCircuitLogPaint.GetValue() != 0;

		if (logPaint)
		{
			VT_LOG(Info, "[Circuit][Frame] window={} bounds=({},{} {}x{})",
				static_cast<const void*>(this),
				windowScreenBounds.GetPosition().x, windowScreenBounds.GetPosition().y,
				windowScreenBounds.GetSize().x, windowScreenBounds.GetSize().y);
		}

		PainterPool pool(windowOrigin, m_resourceTable);
		pool.Reserve(m_windowWidget);

		Volt::TaskGraph paintGraph{ Volt::ExecutionPriority::Immediate };

		Map<Ref<Widget>, Volt::TaskGraph::Task*> widgetPaintTasks;
		auto MakePaintTaskForWidget = [&paintGraph, &widgetPaintTasks, &pool](Ref<Widget> parentWidget, Ref<Widget> inWidget)
		{
			CircuitPainter& painter = pool.GetFor(inWidget);
			VT_ENSURE(inWidget);

			Volt::TaskGraph::Task* paintTask = paintGraph.AddTask("OnPaint Widget",
					[inWidget, &painter]()
			{
				inWidget->OnPaint(painter);
			});

			if (parentWidget)
			{
				Volt::TaskGraph::Task* parentPaintTask = widgetPaintTasks[parentWidget];
				paintTask->AddDependency(parentPaintTask);
			}

			widgetPaintTasks[inWidget] = paintTask;
		};

		CircuitPainter& rootPainter = pool.GetFor(m_windowWidget);
		rootPainter.SetAllottedScreenArea(windowScreenBounds);

		// make tasks from each widget that depends on it's parent's paint job
		//Vector<Ref<Widget>> m_widgets;
		//m_widgets.push_back(m_windowWidget);
		//MakePaintTaskForWidget(nullptr, m_windowWidget);
		//for (size_t i = 0; i < m_widgets.size(); i++)
		//{
		//	Ref<Widget> currParent = m_widgets[i];
		//	m_widgets.append(currParent->GetChildren());

		//	for (Ref<Widget> child : currParent->GetChildren())
		//	{
		//		MakePaintTaskForWidget(currParent, child);
		//	}
		//}

		//temporarily do all widgets sequentually but still using the job system
		const Vector<Ref<Widget>>& reserved = pool.GetReservedWidgets();
		for (size_t i = 0; i < reserved.size(); ++i)
		{
			if (i == 0)
			{
				MakePaintTaskForWidget(nullptr, reserved[i]);
				continue;
			}
			MakePaintTaskForWidget(reserved[i-1], reserved[i]);
		}
		paintGraph.ExecuteAndWait();

		// single threaded version
		//// Paint every reserved widget. BFS order from Reserve guarantees parents paint
		//// before children, so each child's painter has its alloted area set by parent's
		//// AddWidget call before the child's OnPaint runs.
		//{
		//	VT_PROFILE_SCOPE("Circuit::Paint");
		//	Volt::TaskGraph paintGraph{ Volt::ExecutionPriority::Immediate };

		//	const Vector<Ref<Widget>>& reserved = pool.GetReservedWidgets();
		//	for (size_t i = 0; i < reserved.size(); ++i)
		//	{
		//		VT_PROFILE_SCOPE("Circuit::PaintSingleWidget");
		//		const Ref<Widget>& widget = reserved[i];
		//		CircuitPainter& painter = pool.GetFor(widget);
		//		paintGraph.AddTask("OnPaint Widget", [logPaint, i, widget, &painter]()
		//		{
		//			if (logPaint)
		//			{
		//				const Volt::Rect& area = painter.GetAllottedScreenArea();
		//				VT_LOG(Info, "[Circuit][Paint]    idx={} widget={} alloted=({},{} {}x{})",
		//					i, static_cast<const void*>(widget.GetRaw()),
		//					area.GetPosition().x, area.GetPosition().y,
		//					area.GetSize().x, area.GetSize().y);
		//			}
		//			widget->OnPaint(painter);

		//		});
		//	}
		//	paintGraph.ExecuteAndWait();
		//}

		Volt::Rect rootBounds = rootPainter.Consolidate(m_drawCommands);

		m_windowWidget->SetBounds(rootBounds);
		m_windowWidget->SetAllotedScreenArea(windowScreenBounds);

		const bool needsDebugOverlay = s_cvarCircuitShowWidgetBounds.GetValue() || s_cvarCircuitShowHoveredWidgetBounds.GetValue();
		if (needsDebugOverlay)
		{
			PaintDebug();
		}

		if (logPaint)
		{
			VT_LOG(Info, "[Circuit][Frame] total emitted draw cmds: {}", m_drawCommands.size());
		}
	}

	void CircuitWindow::SetWidget(Ref<WindowWidget> widget)
	{
		m_windowWidget = widget;
	}

	void CircuitWindow::OnRender()
	{
		m_renderer->OnRender();
	}

	void CircuitWindow::PaintDebug()
	{
		VT_PROFILE_FUNCTION();

		const Volt::Rect windowScreenBounds = Volt::Rect(static_cast<float>(GetPosition().x), static_cast<float>(GetPosition().y), static_cast<float>(GetSize().x), static_cast<float>(GetSize().y));
		const glm::vec2 windowOrigin = windowScreenBounds.GetPosition();

		CircuitPainter debugPainter(windowOrigin, m_resourceTable);
		debugPainter.SetAllottedScreenArea(windowScreenBounds);

		if (s_cvarCircuitShowWidgetBounds.GetValue())
		{
			Vector<Ref<Widget>> widgetsToCheck{ m_windowWidget };
			while (!widgetsToCheck.empty())
			{
				Ref<Widget> checkingWidget = widgetsToCheck.back();
				widgetsToCheck.pop_back();

				for (Ref<Widget> child : checkingWidget->GetChildren())
				{
					widgetsToCheck.push_back(child);
				}

				const Volt::Rect& screenBounds = checkingWidget->GetBounds();
				const Volt::Rect localBounds(screenBounds.GetPosition() - static_cast<glm::vec2>(GetPosition()), screenBounds.GetSize());
				const CircuitColor boundsColor = 0xff2222ff;
				debugPainter.AddRectOutline(localBounds.GetPosition().x, localBounds.GetPosition().y, localBounds.GetSize().x, localBounds.GetSize().y, boundsColor, 1.f);
			}
		}

		if (s_cvarCircuitShowHoveredWidgetBounds.GetValue())
		{
			Vector<Ref<Widget>> widgetsUnderCursor = CircuitManager::Get().GetInputHandler().GetWidgetsUnderCursor();
			Ref<Widget> hoveredWidget = CircuitManager::Get().GetInputHandler().GetHoveredWidget();

			for (Ref<Widget> widget : widgetsUnderCursor)
			{
				const Volt::Rect& screenBounds = widget->GetBounds();
				const Volt::Rect localBounds(screenBounds.GetPosition() - static_cast<glm::vec2>(GetPosition()), screenBounds.GetSize());
				CircuitColor boundsColor = 0xff2222ff;
				if (widget == hoveredWidget)
				{
					boundsColor = 0x2222ffff;
				}
				debugPainter.AddRectOutline(localBounds.GetPosition().x, localBounds.GetPosition().y, localBounds.GetSize().x, localBounds.GetSize().y, boundsColor, 1.f);
			}
		}

		debugPainter.Consolidate(m_drawCommands);
	}

}
