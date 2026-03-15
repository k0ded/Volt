#pragma once
#include "Circuit/Widgets/CompoundWidget.h"
namespace Circuit
{
	class LayoutWidget;

	class CIRCUIT_API WindowWidget : public CompoundWidget
	{
	public:
		CIRCUIT_BEGIN_ARGS(WindowWidget)
		{
		};

		CIRCUIT_ARGUMENT(Ref<Widget>, Content);

		CIRCUIT_END_ARGS();

		//virtual void OnLayout(const glm::vec2& allotedSize) override;
		virtual glm::vec2 GetDesiredSize() override { return { -1,-1 }; }
		void Build(const Arguments& args);
		virtual void OnPaint(CircuitPainter& painter) override;

		virtual bool IsHittestInvisible() const override { return false; };

	private:
		Ref<LayoutWidget> BuildTitlebar();
		Ref<Widget> m_content;
	};

}
