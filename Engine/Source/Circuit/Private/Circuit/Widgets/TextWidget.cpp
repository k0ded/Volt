#include "circuitpch.h"
#include "Widgets/TextWidget.h"

#include "CircuitPainter.h"

#include <AssetSystem/AssetManager.h>

Circuit::TextWidget::TextWidget() : Circuit::Widget()
{
	m_font = CreateRef<Volt::Font>();
	m_font->Initialize("Engine/Fonts/Futura/futura-light.ttf");
}

Circuit::TextWidget::~TextWidget()
{
}

void Circuit::TextWidget::Build(const Arguments& args)
{
	m_text = args._Text;
	m_size = args._Size;
	m_color = args._Color;
}

glm::vec2 Circuit::TextWidget::OnLayout(const glm::vec2& allotedSize)
{
	const float width = m_font->GetStringWidth(m_text.Get(), m_size, FLT_MAX);
	const float height = m_font->GetStringHeight(m_text.Get(), m_size, FLT_MAX);

	return glm::vec2(width, height);
}

void Circuit::TextWidget::OnPaint(CircuitPainter& painter)
{
	painter.AddText(0, 0, m_text.Get(), m_font, std::numeric_limits<float>().max(), CircuitColor(100, 100, 50), m_size);
}
