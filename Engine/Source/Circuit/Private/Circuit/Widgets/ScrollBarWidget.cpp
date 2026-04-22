#include "circuitpch.h"
#include "Widgets/ScrollBarWidget.h"
#include "Circuit/CircuitPainter.h"
#include "Circuit/WidgetInteractionData.h"

#include <glm/glm.hpp>

namespace Circuit
{
	ScrollBarWidget::ScrollBarWidget()
	{
	}

	ScrollBarWidget::~ScrollBarWidget()
	{
	}

	void ScrollBarWidget::Build(const Arguments& args)
	{
		m_orientation = args._Orientation;
		m_trackColor = args._TrackColor;
		m_thumbColor = args._ThumbColor;
		m_thumbHoverColor = args._ThumbHoverColor;
		m_thumbMinSize = args._ThumbMinSize;

		m_value = args._Value;
		m_thumbSizeNormalized = args._ThumbSizeNormalized;

		m_onValueChanged = args._OnValueChanged;
	}

	glm::vec2 ScrollBarWidget::GetDesiredSize()
	{
		switch (m_orientation)
		{
			case LayoutOrientation::Vertical:
				return { s_scrollBarThickness, -1 };
			case LayoutOrientation::Horizontal:
				return { -1, s_scrollBarThickness };
		}
		return { s_scrollBarThickness, -1 };
	}

	void ScrollBarWidget::OnPaint(CircuitPainter& painter)
	{
		const glm::vec2 size = painter.GetAllottedSize();
		const float trackLength = GetTrackLength(size);
		const float thumbLength = GetThumbLength(trackLength);
		const float thumbOffset = GetThumbOffset(trackLength, thumbLength);

		const CircuitColor& currentThumbColor = m_hovered ? m_thumbHoverColor : m_thumbColor;

		// Draw track
		painter.AddRect(0, 0, size.x, size.y, m_trackColor);

		// Draw thumb
		switch (m_orientation)
		{
			case LayoutOrientation::Vertical:
				painter.AddRect(0, thumbOffset, size.x, thumbLength, currentThumbColor);
				break;
			case LayoutOrientation::Horizontal:
				painter.AddRect(thumbOffset, 0, thumbLength, size.y, currentThumbColor);
				break;
		}
	}

	void ScrollBarWidget::OnPressed(const WidgetInteractionData& interactionData)
	{
		if (interactionData.mouseButton != Volt::InputCode::Mouse_LB)
		{
			return;
		}
		SetValueFromMousePos(interactionData.mousePos);
	}

	void ScrollBarWidget::OnBeginDrag(const WidgetInteractionData& interactionData)
	{
		if (interactionData.mouseButton != Volt::InputCode::Mouse_LB)
		{
			return;
		}
		m_dragging = true;
	}

	void ScrollBarWidget::OnDrag(const WidgetInteractionData& interactionData)
	{
		if (interactionData.mouseButton != Volt::InputCode::Mouse_LB)
		{
			return;
		}
		SetValueFromMousePos(interactionData.mousePos);
	}

	void ScrollBarWidget::OnEndDrag(const WidgetInteractionData& interactionData)
	{
		if (interactionData.mouseButton != Volt::InputCode::Mouse_LB)
		{
			return;
		}
		m_dragging = false;
	}

	void ScrollBarWidget::OnBeginHover(const WidgetInteractionData& interactionData)
	{
		m_hovered = true;
	}

	void ScrollBarWidget::OnEndHover(const WidgetInteractionData& interactionData)
	{
		m_hovered = false;
	}

	void ScrollBarWidget::SetValueFromMousePos(const glm::vec2& mouseScreenPos)
	{
		const Volt::Rect& bounds = GetBounds();
		const glm::vec2 topLeft = bounds.GetPosition();
		const glm::vec2 bottomRight = bounds.GetBottomRight();

		float normalized = 0.f;

		switch (m_orientation)
		{
			case LayoutOrientation::Vertical:
			{
				const float clamped = bounds.ClampInsideY(mouseScreenPos.y);
				normalized = (clamped - topLeft.y) / (bottomRight.y - topLeft.y);
				break;
			}
			case LayoutOrientation::Horizontal:
			{
				const float clamped = bounds.ClampInsideX(mouseScreenPos.x);
				normalized = (clamped - topLeft.x) / (bottomRight.x - topLeft.x);
				break;
			}
		}

		normalized = glm::clamp(normalized, 0.f, 1.f);
		m_onValueChanged.ExecuteIfBound(normalized);
	}

	float ScrollBarWidget::GetTrackLength(const glm::vec2& allottedSize) const
	{
		switch (m_orientation)
		{
			case LayoutOrientation::Vertical:
				return allottedSize.y;
			case LayoutOrientation::Horizontal:
				return allottedSize.x;
		}
		return allottedSize.y;
	}

	float ScrollBarWidget::GetThumbLength(float trackLength) const
	{
		const float thumbNormalized = glm::clamp(m_thumbSizeNormalized.Get(), 0.01f, 1.f);
		return glm::max(trackLength * thumbNormalized, m_thumbMinSize);
	}

	float ScrollBarWidget::GetThumbOffset(float trackLength, float thumbLength) const
	{
		const float scrollableRange = trackLength - thumbLength;
		const float value = glm::clamp(m_value.Get(), 0.f, 1.f);
		return value * scrollableRange;
	}
}
