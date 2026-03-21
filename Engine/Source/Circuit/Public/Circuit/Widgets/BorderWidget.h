#pragma once

#include "Circuit/Widgets/CompoundWidget.h"
#include "Circuit/CircuitColor.h"

namespace Circuit
{
	class CIRCUIT_API BorderWidget : public CompoundWidget
	{
	public:
		BorderWidget();
		virtual ~BorderWidget();

		CIRCUIT_BEGIN_ARGS(BorderWidget)
			: _BackgroundColor(0x00000000),
			_Padding(0.f, 0.f, 0.f, 0.f)
		{
		};

		CIRCUIT_ARGUMENT(CircuitColor, BackgroundColor);
		CIRCUIT_ARGUMENT(Ref<Widget>, Content);

		// Padding: left, top, right, bottom
		CIRCUIT_ARGUMENT(glm::vec4, Padding);

		CIRCUIT_END_ARGS();

		void Build(const Arguments& args);

		virtual glm::vec2 GetDesiredSize() override;
		virtual void OnPaint(CircuitPainter& painter) override;

		virtual bool IsHittestInvisible() const override { return true; };

	private:
		CircuitColor m_backgroundColor;
		Ref<Widget> m_content;
		glm::vec4 m_padding;
	};
}
