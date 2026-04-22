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

void Circuit::BorderWidget::OnPaint(CircuitPainter& painter)
{
	const float left = m_padding.x;
	const float top = m_padding.y;
	const float right = m_padding.z;
	const float bottom = m_padding.w;

	const glm::vec2 size = painter.GetAllottedSize();
	painter.AddRect(left, top, size.x - left - right, size.y - top - bottom, m_backgroundColor.Get());

	if (m_content)
	{
		painter.AddWidget(m_content, left, top, size.x - left - right, size.y - top - bottom);
	}
}
