#include "circuitpch.h"
#include "Widgets/BorderWidget.h"
#include "CircuitPainter.h"

Circuit::BorderWidget::BorderWidget()
{}

Circuit::BorderWidget::~BorderWidget()
{}

void Circuit::BorderWidget::Build(const Arguments& args)
{
	m_backgroundColor = args._BackgroundColor;
	m_content = args._Content;
	m_padding = args._Padding;

	if (m_content)
	{
		AddChildWidget(m_content);
	}
}

glm::vec2 Circuit::BorderWidget::GetDesiredSize()
{
	return { -1, -1 };
}

void Circuit::BorderWidget::OnPaint(CircuitPainter& painter)
{
	const glm::vec2 size = painter.GetAllotedSize();
	painter.AddRect(0, 0, size.x, size.y, m_backgroundColor);

	if (m_content)
	{
		const float left = m_padding.x;
		const float top = m_padding.y;
		const float right = m_padding.z;
		const float bottom = m_padding.w;

		painter.AddWidget(m_content, left, top, size.x - left - right, size.y - top - bottom);
	}
}
