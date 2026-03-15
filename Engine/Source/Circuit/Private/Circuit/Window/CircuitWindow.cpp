#include "circuitpch.h"
#include "Window/CircuitWindow.h"
#include "Circuit/CircuitPainter.h"

#include "Circuit/Widgets/Widget.h"
#include "Circuit/Widgets/WindowWidget.h"

#include "Circuit/Rendering/CircuitRenderer.h"

#include "Circuit/ConsoleVars.h"

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>
#include <WindowModule/Events/WindowEvents.h>

#include <LogModule/Log.h>

namespace Circuit
{
	CircuitWindow::CircuitWindow(Volt::WindowHandle windowHandle)
		: m_windowHandle(windowHandle)
	{
		m_renderer = CreateRef<CircuitRenderer>(*this);
		RegisterEventListeners();
	}

	void CircuitWindow::RegisterEventListeners()
	{
		RegisterListener<Volt::WindowTitlebarHittestEvent>(VT_BIND_EVENT_FN(CircuitWindow::OnWindowTitlebarHittestEvent));
	}
	bool CircuitWindow::OnWindowTitlebarHittestEvent(Volt::WindowTitlebarHittestEvent& e)
	{
		if (e.GetWindow().GetHandle() == m_windowHandle)
		{
			e.SetHit(m_windowWidget->IsHoveringTitlebar());
			if (m_windowWidget->IsHoveringTitlebar())
			{
				VT_LOG(Info, "Hovering Titlebar!");
			}
		}

		return false;
	}

	Volt::WindowHandle CircuitWindow::GetWindowHandle() const
	{
		return m_windowHandle;
	}

	CIRCUIT_API glm::i32vec2 CircuitWindow::GetPosition() const
	{
		Volt::Window& window = Volt::WindowManager::Get().GetWindow(m_windowHandle);
		return { window.GetPosition().first, window.GetPosition().second };
	}

	glm::u32vec2 CircuitWindow::GetSize() const
	{
		Volt::Window& window = Volt::WindowManager::Get().GetWindow(m_windowHandle);
		return { window.GetWidth(), window.GetHeight() };
	}
	void CircuitWindow::Resize(const glm::vec2& size)
	{
		m_windowSize = size;
	}

	std::vector<CircuitDrawCommand> CircuitWindow::GetDrawCommands()
	{
		const Volt::Rect windowScreenBounds = Volt::Rect(static_cast<float>(GetPosition().x), static_cast<float>(GetPosition().y), static_cast<float>(GetSize().x), static_cast<float>(GetSize().y));
		CircuitPainter basePainter(windowScreenBounds);
		if (m_windowWidget)
		{
			const Volt::Rect windowLocalBounds = Volt::Rect(0.f, 0.f, static_cast<float>(GetSize().x), static_cast<float>(GetSize().y));
			basePainter.AddWidget(m_windowWidget, windowLocalBounds);


			if (s_cvarCircuitShowWidgetBounds.GetValue())
			{
				//draw widget bounds
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
					const CircuitColor boundsColor = 0xff000050;
					basePainter.AddRect(localBounds.GetPosition().x, localBounds.GetPosition().y, localBounds.GetSize().x, localBounds.GetSize().y, boundsColor);
				}
			}
		}

		return basePainter.GetCommands();
	}

	void CircuitWindow::SetWidget(Ref<WindowWidget> widget)
	{
		m_windowWidget = widget;
	}

	void CircuitWindow::OnRender()
	{
		m_renderer->OnRender();
	}

}
