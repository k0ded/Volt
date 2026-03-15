#include "circuitpch.h"
#include "WindowWidget.h"

#include "Circuit/Widgets/Layout/LayoutWidget.h"
#include "Circuit/Widgets/ButtonWidget.h"
#include "TextWidget.h"

#include "Circuit/CircuitPainter.h"

namespace Circuit
{
	void WindowWidget::Build(const Arguments& args)
	{
		m_content = args._Content;

		Ref<LayoutWidget> layout = CreateWidget(LayoutWidget).Orientation(LayoutOrientation::Vertical);

		constexpr float titlebarHeight = 50.f;
		constexpr float windowIconSize = 40.f;
		m_titlebar = CreateWidget(WindowTitlebarWidget)
			.Height(titlebarHeight)
			.IconSize(windowIconSize)
			.Color(0x555560ff);

		layout->AddFixedSlice(m_titlebar, titlebarHeight);

		if (m_content)
		{
			layout->AddFlexibleSlice(m_content);
		}

		AddChildWidget(layout);
	}

	void WindowWidget::OnPaint(CircuitPainter& painter)
	{
		const CircuitColor windowBgColor(59, 59, 59);

		painter.AddRect(0, 0, painter.GetAllotedSize().x, painter.GetAllotedSize().y, windowBgColor);

		CompoundWidget::OnPaint(painter);
	}

	bool WindowWidget::IsHoveringTitlebar() const
	{
		if (m_titlebar)
		{
			return m_titlebar->IsHoveringTitlebar();
		}
		return false;
	}
}
