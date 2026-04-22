#pragma once
#include "Circuit/Widgets/CompoundWidget.h"
#include "Circuit/Widgets/ScrollBarWidget.h"
#include "Circuit/CircuitColor.h"

#include <glm/vec2.hpp>

namespace Circuit
{
	class BorderWidget;

	class CIRCUIT_API ScrollBoxWidget : public CompoundWidget
	{
	public:
		ScrollBoxWidget();
		virtual ~ScrollBoxWidget();

		CIRCUIT_BEGIN_ARGS(ScrollBoxWidget)
			: _AllowVerticalScroll(true)
			, _AllowHorizontalScroll(false)
			, _ScrollBarThickness(12.f)
			, _BackgroundColor(0x00000000)
			, _ScrollBarTrackColor(0x333333ff)
			, _ScrollBarThumbColor(0x888888ff)
			, _ScrollBarThumbHoverColor(0xaaaaaaff)
			, _ScrollSpeed(30.f)
			, _ContentSize(glm::vec2(-1, -1))
		{
		};

		CIRCUIT_ARGUMENT(bool, AllowVerticalScroll);
		CIRCUIT_ARGUMENT(bool, AllowHorizontalScroll);
		CIRCUIT_ARGUMENT(float, ScrollBarThickness);
		CIRCUIT_ARGUMENT(CircuitColor, BackgroundColor);
		CIRCUIT_ARGUMENT(CircuitColor, ScrollBarTrackColor);
		CIRCUIT_ARGUMENT(CircuitColor, ScrollBarThumbColor);
		CIRCUIT_ARGUMENT(CircuitColor, ScrollBarThumbHoverColor);
		CIRCUIT_ARGUMENT(float, ScrollSpeed);

		CIRCUIT_ATTRIBUTE(glm::vec2, ContentSize);

		CIRCUIT_END_ARGS();

		void Build(const Arguments& args);

		virtual glm::vec2 GetDesiredSize() override;
		virtual void OnPaint(CircuitPainter& painter) override;

		virtual void OnScrolled(const WidgetInteractionData& interactionData) override;

		virtual bool IsHittestInvisible() const override { return false; }

		float GetVerticalScrollOffset() const { return m_verticalScrollOffset; }
		float GetHorizontalScrollOffset() const { return m_horizontalScrollOffset; }

		void SetVerticalScrollOffset(float offset);
		void SetHorizontalScrollOffset(float offset);

	private:
		void OnVerticalScrollBarValueChanged(float newValue);
		void OnHorizontalScrollBarValueChanged(float newValue);

		Ref<BorderWidget> m_backgroundBox;
		Ref<ScrollBarWidget> m_verticalScrollBar;
		Ref<ScrollBarWidget> m_horizontalScrollBar;

		bool m_allowVerticalScroll;
		bool m_allowHorizontalScroll;
		float m_scrollBarThickness;
		CircuitColor m_backgroundColor;
		float m_scrollSpeed;

		float m_verticalScrollOffset = 0.f;
		float m_horizontalScrollOffset = 0.f;

		float m_verticalThumbSize = 1.f;
		float m_horizontalThumbSize = 1.f;

		Volt::Attribute<glm::vec2> m_contentSize;
	};
}
