#pragma once
#include "Circuit/Widgets/Widget.h"
#include "Circuit/Widgets/Layout/LayoutWidget.h"
#include "Circuit/CircuitColor.h"

#include <CoreUtilities/Delegates/DelegateDeclarationHelpers.h>

DECLARE_DELEGATE_OneParam(OnScrollBarValueChangedDelegate, float /*NewValue*/);

namespace Circuit
{
	class CIRCUIT_API ScrollBarWidget : public Widget
	{
	public:
		ScrollBarWidget();
		virtual ~ScrollBarWidget();

		CIRCUIT_BEGIN_ARGS(ScrollBarWidget)
			: _Orientation(LayoutOrientation::Vertical)
			, _TrackColor(0x333333ff)
			, _ThumbColor(0x888888ff)
			, _ThumbHoverColor(0xaaaaaaff)
			, _ThumbMinSize(20.f)
		{
		};

		CIRCUIT_ATTRIBUTE(float, Value);
		CIRCUIT_ATTRIBUTE(float, ThumbSizeNormalized);

		CIRCUIT_ARGUMENT(LayoutOrientation, Orientation);
		CIRCUIT_ARGUMENT(CircuitColor, TrackColor);
		CIRCUIT_ARGUMENT(CircuitColor, ThumbColor);
		CIRCUIT_ARGUMENT(CircuitColor, ThumbHoverColor);
		CIRCUIT_ARGUMENT(float, ThumbMinSize);

		CIRCUIT_EVENT(OnScrollBarValueChangedDelegate, OnValueChanged);

		CIRCUIT_END_ARGS();

		void Build(const Arguments& args);

		virtual glm::vec2 GetDesiredSize() override;
		virtual void OnPaint(CircuitPainter& painter) override;

		virtual void OnPressed(const WidgetInteractionData& interactionData) override;
		virtual void OnBeginDrag(const WidgetInteractionData& interactionData) override;
		virtual void OnDrag(const WidgetInteractionData& interactionData) override;
		virtual void OnEndDrag(const WidgetInteractionData& interactionData) override;

		virtual void OnBeginHover(const WidgetInteractionData& interactionData) override;
		virtual void OnEndHover(const WidgetInteractionData& interactionData) override;

	private:
		void SetValueFromMousePos(const glm::vec2& mouseScreenPos);
		float GetTrackLength(const glm::vec2& allottedSize) const;
		float GetThumbLength(float trackLength) const;
		float GetThumbOffset(float trackLength, float thumbLength) const;

		Volt::Attribute<float> m_value;
		Volt::Attribute<float> m_thumbSizeNormalized;

		LayoutOrientation m_orientation;
		CircuitColor m_trackColor;
		CircuitColor m_thumbColor;
		CircuitColor m_thumbHoverColor;
		float m_thumbMinSize;

		OnScrollBarValueChangedDelegate m_onValueChanged;

		bool m_hovered = false;
		bool m_dragging = false;

		static constexpr float s_scrollBarThickness = 12.f;
	};
}
