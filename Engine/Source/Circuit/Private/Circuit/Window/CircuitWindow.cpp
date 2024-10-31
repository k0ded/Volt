#include "circuitpch.h"
#include "Window/CircuitWindow.h"
#include "Circuit/CircuitPainter.h"

#include "Circuit/Rendering/CircuitRenderer.h"

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

namespace Circuit
{
	CircuitWindow::CircuitWindow(Volt::WindowHandle windowHandle)
		: m_windowHandle(windowHandle)
	{
		m_renderer = CreateRef<CircuitRenderer>(*this);
	}

	Volt::WindowHandle CircuitWindow::GetWindowHandle() const
	{
		return m_windowHandle;
	}

	CIRCUIT_API glm::u32vec2 CircuitWindow::GetPosition() const
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
		CircuitPainter painter(glm::vec2(0,0), glm::vec2(GetSize().x, GetSize().y));
		if (m_widget)
		{
			m_widget->OnPaint(painter);
		}
		return painter.GetCommands();
	}

	void CircuitWindow::SetWidget(Ref<Widget> widget)
	{
		m_widget = widget;
	}

	void CircuitWindow::OnRender()
	{
		m_renderer->OnRender();
	}
}
