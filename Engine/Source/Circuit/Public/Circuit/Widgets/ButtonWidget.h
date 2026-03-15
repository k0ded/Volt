#pragma once

#include "Circuit/Widgets/CompoundWidget.h"

namespace Circuit
{
	class CIRCUIT_API ButtonWidget : public CompoundWidget
	{
	public:
		ButtonWidget();
		virtual ~ButtonWidget();

		CIRCUIT_BEGIN_ARGS(ButtonWidget)
			: _MinSize(5, 5)
		{
		};

		CIRCUIT_ARGUMENT(glm::vec2, MinSize);

		CIRCUIT_ARGUMENT(Ref<Widget>, Content);

		CIRCUIT_END_ARGS();

		void Build(const Arguments& args);

		virtual void OnLayout(const glm::vec2& allotedSize) override;
		virtual glm::vec2 GetDesiredSize() override;

		virtual void OnPaint(CircuitPainter& painter) override;


		virtual void OnBeginHover(const WidgetInteractionData& interactionData) override;
		virtual void OnEndHover(const WidgetInteractionData& interactionData) override;
		virtual void OnPressed(const WidgetInteractionData& interactionData) override;
		virtual void OnReleased(const WidgetInteractionData& interactionData) override;
	private:
		Ref<Widget> m_content;

		bool m_hovered;
		bool m_pressed;

		glm::vec2 m_minSize;

		glm::vec2 m_size;
	};
}
