#include "circuitpch.h"
#include "Widgets/ButtonWidget.h"

#include "CircuitPainter.h"

#include <InputModule/Input.h>
#include <InputModule/Events/MouseEvents.h>

Circuit::ButtonWidget::ButtonWidget()
{}

Circuit::ButtonWidget::~ButtonWidget()
{}

void Circuit::ButtonWidget::Build(const Arguments& args)
{
	m_content = args._Content;

	if (m_content)
	{
		AddChildWidget(m_content);
	}

	m_hovered = false;
	m_pressed = false;
	m_minSize = args._MinSize;
}

glm::vec2 Circuit::ButtonWidget::GetDesiredSize()
{
	glm::vec2 result;

	if (m_content)
	{
		result = m_content->GetDesiredSize();
	}
	else
	{
		//if we dont have content, take the whole area given
		result = { -1,-1 };
	}

	m_size = result;
	return result;
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
	painter.AddRect(0, 0, painter.GetAllotedSize().x, painter.GetAllotedSize().y, *buttonColor);

	if (m_content)
	{
		painter.AddWidget(m_content, 0, 0, painter.GetAllotedSize().x, painter.GetAllotedSize().y);
	}
}

void Circuit::ButtonWidget::OnBeginHover(const WidgetInteractionData& interactionData)
{
	m_hovered = true;
}

void Circuit::ButtonWidget::OnEndHover(const WidgetInteractionData& interactionData)
{
	m_hovered = false;
	m_pressed = false;
}

void Circuit::ButtonWidget::OnPressed(const WidgetInteractionData& interactionData)
{
	m_pressed = true;
}

void Circuit::ButtonWidget::OnReleased(const WidgetInteractionData& interactionData)
{
	m_pressed = false;
}
