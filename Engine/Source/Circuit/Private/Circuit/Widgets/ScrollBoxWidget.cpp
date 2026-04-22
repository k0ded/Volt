#include "circuitpch.h"
#include "Widgets/ScrollBoxWidget.h"
#include "Widgets/BorderWidget.h"
#include "Circuit/CircuitPainter.h"
#include "Circuit/WidgetInteractionData.h"

#include <glm/glm.hpp>

namespace Circuit
{
	ScrollBoxWidget::ScrollBoxWidget()
	{
	}

	ScrollBoxWidget::~ScrollBoxWidget()
	{
	}

	void ScrollBoxWidget::Build(const Arguments& args)
	{
		m_allowVerticalScroll = args._AllowVerticalScroll;
		m_allowHorizontalScroll = args._AllowHorizontalScroll;
		m_scrollBarThickness = args._ScrollBarThickness;
		m_backgroundColor = args._BackgroundColor;
		m_scrollSpeed = args._ScrollSpeed;
		m_contentSize = args._ContentSize;

		m_backgroundBox = CreateWidget(BorderWidget)
			.BackgroundColor(args._BackgroundColor);

		AddChildWidget(m_backgroundBox);

		if (m_allowVerticalScroll)
		{
			m_verticalScrollBar = CreateWidget(ScrollBarWidget)
				.Orientation(LayoutOrientation::Vertical)
				.TrackColor(args._ScrollBarTrackColor)
				.ThumbColor(args._ScrollBarThumbColor)
				.ThumbHoverColor(args._ScrollBarThumbHoverColor)
				.Value_Lambda([this]() { return m_verticalScrollOffset; })
				.ThumbSizeNormalized_Lambda([this]() { return m_verticalThumbSize; })
				.OnValueChanged_Raw(this, &ScrollBoxWidget::OnVerticalScrollBarValueChanged);

			AddChildWidget(m_verticalScrollBar);
		}

		if (m_allowHorizontalScroll)
		{
			m_horizontalScrollBar = CreateWidget(ScrollBarWidget)
				.Orientation(LayoutOrientation::Horizontal)
				.TrackColor(args._ScrollBarTrackColor)
				.ThumbColor(args._ScrollBarThumbColor)
				.ThumbHoverColor(args._ScrollBarThumbHoverColor)
				.Value_Lambda([this]() { return m_horizontalScrollOffset; })
				.ThumbSizeNormalized_Lambda([this]() { return m_horizontalThumbSize; })
				.OnValueChanged_Raw(this, &ScrollBoxWidget::OnHorizontalScrollBarValueChanged);

			AddChildWidget(m_horizontalScrollBar);
		}
	}

	glm::vec2 ScrollBoxWidget::GetDesiredSize()
	{
		return { -1, -1 };
	}

	void ScrollBoxWidget::OnPaint(CircuitPainter& painter)
	{
		const glm::vec2 allottedSize = painter.GetAllottedSize();

		// Determine content desired size
		glm::vec2 contentDesiredSize = m_contentSize.Get();

		// Replace -1 (fill) with allotted size
		const glm::vec2 contentSize = {
			contentDesiredSize.x > 0 ? contentDesiredSize.x : allottedSize.x,
			contentDesiredSize.y > 0 ? contentDesiredSize.y : allottedSize.y
		};

		// Calculate visible area, accounting for scrollbar space
		float visibleWidth = allottedSize.x;
		float visibleHeight = allottedSize.y;

		const bool needsVerticalBar = m_allowVerticalScroll && contentSize.y > visibleHeight;
		const bool needsHorizontalBar = m_allowHorizontalScroll && contentSize.x > visibleWidth;

		if (needsVerticalBar)
		{
			visibleWidth -= m_scrollBarThickness;
		}
		if (needsHorizontalBar)
		{
			visibleHeight -= m_scrollBarThickness;
		}

		// Update thumb sizes based on visible/content ratio
		if (needsVerticalBar)
		{
			m_verticalThumbSize = glm::clamp(visibleHeight / contentSize.y, 0.01f, 1.f);
		}
		if (needsHorizontalBar)
		{
			m_horizontalThumbSize = glm::clamp(visibleWidth / contentSize.x, 0.01f, 1.f);
		}

		// Paint background box
		painter.AddWidget(m_backgroundBox, 0, 0, visibleWidth, visibleHeight);

		// Paint vertical scrollbar
		if (needsVerticalBar && m_verticalScrollBar)
		{
			painter.AddWidget(m_verticalScrollBar, visibleWidth, 0, m_scrollBarThickness, visibleHeight);
		}

		// Paint horizontal scrollbar
		if (needsHorizontalBar && m_horizontalScrollBar)
		{
			painter.AddWidget(m_horizontalScrollBar, 0, visibleHeight, visibleWidth, m_scrollBarThickness);
		}
	}

	void ScrollBoxWidget::OnScrolled(const WidgetInteractionData& interactionData)
	{
		if (m_allowVerticalScroll)
		{
			const glm::vec2 allottedSize = GetAllotedScreenArea().GetSize();
			const glm::vec2 contentDesiredSize = m_contentSize.Get();
			const float contentHeight = contentDesiredSize.y > 0 ? contentDesiredSize.y : allottedSize.y;
			const float maxScrollY = glm::max(1.f, contentHeight - allottedSize.y);

			const float scrollDeltaNormalized = (interactionData.scrollDelta.y * m_scrollSpeed) / maxScrollY;
			SetVerticalScrollOffset(m_verticalScrollOffset - scrollDeltaNormalized);
		}

		if (m_allowHorizontalScroll)
		{
			const glm::vec2 allottedSize = GetAllotedScreenArea().GetSize();
			const glm::vec2 contentDesiredSize = m_contentSize.Get();
			const float contentWidth = contentDesiredSize.x > 0 ? contentDesiredSize.x : allottedSize.x;
			const float maxScrollX = glm::max(1.f, contentWidth - allottedSize.x);

			const float scrollDeltaNormalized = (interactionData.scrollDelta.x * m_scrollSpeed) / maxScrollX;
			SetHorizontalScrollOffset(m_horizontalScrollOffset - scrollDeltaNormalized);
		}
	}

	void ScrollBoxWidget::SetVerticalScrollOffset(float offset)
	{
		m_verticalScrollOffset = glm::clamp(offset, 0.f, 1.f);
	}

	void ScrollBoxWidget::SetHorizontalScrollOffset(float offset)
	{
		m_horizontalScrollOffset = glm::clamp(offset, 0.f, 1.f);
	}

	void ScrollBoxWidget::OnVerticalScrollBarValueChanged(float newValue)
	{
		SetVerticalScrollOffset(newValue);
	}

	void ScrollBoxWidget::OnHorizontalScrollBarValueChanged(float newValue)
	{
		SetHorizontalScrollOffset(newValue);
	}
}
