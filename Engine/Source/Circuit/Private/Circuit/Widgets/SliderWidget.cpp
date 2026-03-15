#include "circuitpch.h"

#include "Widgets/SliderWidget.h"

#include "Circuit/CircuitColor.h"

#include "Circuit/CircuitPainter.h"
#include "Circuit/WidgetInteractionData.h"

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

#include <InputModule/Events/MouseEvents.h>


namespace Circuit
{

	SliderWidget::SliderWidget()
	{
		m_dragging = false;
	}
	SliderWidget::~SliderWidget()
	{
		//m_Value->GetOnChangeDelegate().Remove(m_OnValueChangeHandle);

	}

	void SliderWidget::Build(const Arguments& args)
	{
		m_minValue = args._MinValue;
		m_maxValue = args._MaxValue;

		m_value = args._Value;

		m_onValueChanged = args._OnValueChanged;
	}

	glm::vec2 SliderWidget::GetDesiredSize()
	{
		return { -1, s_sliderHeight };
	}

	void SliderWidget::OnPaint(CircuitPainter& painter)
	{
		const float sliderWidth = painter.GetAllotedSize().x;
		const float leftRectWidth = sliderWidth * GetValueNormalized();

		const CircuitColor unfilledColor = 0x555555ff;
		const CircuitColor filledColor = 0xffa500ff;
		const CircuitColor handleColor = 0x000000ff;


		//left rect
		painter.AddRect(0, 0, leftRectWidth, s_sliderHeight, filledColor);

		//right rect
		painter.AddRect(leftRectWidth, 0, sliderWidth - leftRectWidth, s_sliderHeight, unfilledColor);

		//handle outer
		painter.AddCircle(leftRectWidth, s_sliderHeight / 2, s_sliderHandleRadius, handleColor);

		//handle inner
		painter.AddCircle(leftRectWidth, s_sliderHeight / 2, (s_sliderHeight / 3), filledColor);

	}

	float SliderWidget::GetValue() const
	{
		return m_value.Get();
	}

	float SliderWidget::GetMinValue() const
	{
		return m_minValue;
	}

	float SliderWidget::GetMaxValue() const
	{
		return m_maxValue;
	}

	float SliderWidget::GetValueNormalized()
	{
		return (m_value.Get() - m_minValue) / (m_maxValue - m_minValue);
	}

	void SliderWidget::OnPressed(const WidgetInteractionData& interactionData)
	{
		if (interactionData.mouseButton != Volt::InputCode::Mouse_LB)
		{
			return;
		}
		SetValueAccordingToMousePos(interactionData.mousePos);
	}

	void SliderWidget::OnBeginDrag(const WidgetInteractionData& interactionData)
	{
		if (interactionData.mouseButton != Volt::InputCode::Mouse_LB)
		{
			return;
		}
		m_dragging = true;
	}

	void SliderWidget::OnDrag(const WidgetInteractionData & interactionData)
	{
		if (interactionData.mouseButton != Volt::InputCode::Mouse_LB)
		{
			return;
		}
		SetValueAccordingToMousePos(interactionData.mousePos);
	}

	void SliderWidget::OnEndDrag(const WidgetInteractionData & interactionData)
	{
		if (interactionData.mouseButton != Volt::InputCode::Mouse_LB)
		{
			return;
		}
		m_dragging = false;
	}

	void SliderWidget::SetValueAccordingToMousePos(const glm::vec2& mouseScreenPos)
	{
		float clamped = GetBounds().ClampInsideX(mouseScreenPos.x);

		const glm::vec2 topLeft = GetBounds().GetPosition();
		const glm::vec2 bottomRight = GetBounds().GetBottomRight();

		// Calculate the normalized value (0 to 1)
		float valueNormalized = (clamped - topLeft.x) / (bottomRight.x - topLeft.x);

		m_onValueChanged.ExecuteIfBound(m_minValue + valueNormalized * (m_maxValue - m_minValue));
	}
}
