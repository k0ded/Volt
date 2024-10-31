#include "circuitpch.h"
#include "Widgets/ButtonWidget.h"

#include "CircuitPainter.h"

#include <InputModule/Input.h>
#include <InputModule/Events/MouseEvents.h>

Circuit::ButtonWidget::ButtonWidget()
{
}

Circuit::ButtonWidget::~ButtonWidget()
{
}

void Circuit::ButtonWidget::Build(const Arguments& args)
{
	m_hovered = false;
	m_pressed = false;
	m_minSize = args._MinSize;
	m_size = args._Size;
}

void Circuit::ButtonWidget::OnPaint(CircuitPainter& painter)
{
	const CircuitColor baseColor(255, 255, 255);
	const CircuitColor hoveredColor(200, 200, 200);
	const CircuitColor pressedColor(150, 150, 150);

	const CircuitColor* buttonColor = &baseColor;
	if (m_pressed)
	{
		buttonColor = &pressedColor;
	}
	else if (m_hovered)
	{
		buttonColor = &hoveredColor;
	}
	glm::vec2 size = m_size;
	//if we have an invalid size, autosize instead
	if (size.x < 0 || size.y < 0)
	{
		size = glm::vec2(glm::max(painter.GetAllotedArea().GetSize().x, m_minSize.x), glm::max(painter.GetAllotedArea().GetSize().y, m_minSize.y));
	}
	painter.AddRect(0, 0, size.x, size.y, *buttonColor);
}

void Circuit::ButtonWidget::OnBeginHover()
{
	m_hovered = true;
}

void Circuit::ButtonWidget::OnEndHover()
{
	m_hovered = false;
	m_pressed = false;
}

void Circuit::ButtonWidget::OnPressed()
{
	m_pressed = true;
}

void Circuit::ButtonWidget::OnReleased()
{
	m_pressed = false;
}
