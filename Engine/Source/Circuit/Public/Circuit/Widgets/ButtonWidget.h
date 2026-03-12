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

		virtual glm::vec2 OnLayout(const glm::vec2& allotedSize) override;
		virtual void OnPaint(CircuitPainter& painter) override;


		virtual void OnBeginHover();
		virtual void OnEndHover();
		virtual void OnPressed();
		virtual void OnReleased();
	private:
		Ref<Widget> m_content;

		bool m_hovered;
		bool m_pressed;

		glm::vec2 m_minSize;

		glm::vec2 m_size;
	};
}
