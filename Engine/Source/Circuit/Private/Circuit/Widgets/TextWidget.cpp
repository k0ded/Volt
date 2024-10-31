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

void Circuit::TextWidget::OnPaint(CircuitPainter& painter)
{
	const glm::vec2 painterPos = painter.GetAllotedArea().GetPosition();
	painter.AddText(painterPos.x, painterPos.y, m_text.Get(), m_font, std::numeric_limits<float>().max(), CircuitColor(100, 100, 50), m_size);
}
