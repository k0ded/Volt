#pragma once
#include "Circuit/Widgets/CompoundWidget.h"

#include "Circuit/CircuitColor.h"

namespace Circuit
{
	class LayoutWidget;

	class CIRCUIT_API WindowTitlebarWidget : public CompoundWidget
	{
	public:
		CIRCUIT_BEGIN_ARGS(WindowTitlebarWidget)
			: _Color(30)
			, _IconSize(40)
			, _Height(50)
		{
		};

		CIRCUIT_ARGUMENT(CircuitColor, Color);
		CIRCUIT_ARGUMENT(float, IconSize);
		CIRCUIT_ARGUMENT(float, Height);

		CIRCUIT_END_ARGS();

		virtual glm::vec2 GetDesiredSize() override;
		void Build(const Arguments& args);
		virtual void OnPaint(CircuitPainter& painter) override;

		bool IsHoveringTitlebar() const { return m_titlebarHovered; }

		virtual void OnBeginHover(const WidgetInteractionData& interactionData) override;
		virtual void OnEndHover(const WidgetInteractionData& interactionData) override;
	private:
		Ref<Widget> CreateMinimizeButton();
		Ref<Widget> CreateMaximizeButton();
		Ref<Widget> CreateCloseButton();

		float m_iconSize;
		float m_height;

		bool m_titlebarHovered = false;
		CircuitColor m_titlebarColor;
	};

}
