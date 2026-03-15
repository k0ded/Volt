#pragma once
#include "Circuit/Widgets/Widget.h"

#include <CoreUtilities/Delegates/Delegate.h>

#include <EventSystem/EventListener.h>


namespace Volt
{
	class MouseMovedEvent;
	class MouseButtonPressedEvent;
	class MouseButtonReleasedEvent;
}

DECLARE_DELEGATE_OneParam(OnFloatValueChangedDelegate, float /*NewValue*/);
namespace Circuit
{
	class CIRCUIT_API SliderWidget : public Widget
	{
	public:
		SliderWidget();
		virtual ~SliderWidget();

		CIRCUIT_BEGIN_ARGS(SliderWidget)
		{
		};

		CIRCUIT_ATTRIBUTE(float, Value);

		CIRCUIT_ARGUMENT(float, MinValue);
		CIRCUIT_ARGUMENT(float, MaxValue);

		CIRCUIT_EVENT(OnFloatValueChangedDelegate, OnValueChanged)
		CIRCUIT_END_ARGS();

		void Build(const Arguments& args);

		virtual void OnLayout(const glm::vec2& allotedSize) override;

		virtual glm::vec2 GetDesiredSize() override;
		virtual void OnPaint(CircuitPainter& painter) override;

		float GetValue() const;
		float GetMinValue() const;
		float GetMaxValue() const;

		float GetValueNormalized();

		virtual void OnPressed(const WidgetInteractionData& interactionData) override;
		virtual void OnBeginDrag(const WidgetInteractionData& interactionData);
		virtual void OnDrag(const WidgetInteractionData& interactionData);
		virtual void OnEndDrag(const WidgetInteractionData& interactionData);

	private:

		void SetValueAccordingToMousePos(const glm::vec2& mouseScreenPos);

		bool m_dragging;

		Volt::Attribute<float> m_value;

		float m_minValue;
		float m_maxValue;

		OnFloatValueChangedDelegate m_onValueChanged;

		static constexpr uint32_t s_sliderHeight = 10;
		static constexpr uint32_t s_sliderHandleRadius = 10;
	};
}
