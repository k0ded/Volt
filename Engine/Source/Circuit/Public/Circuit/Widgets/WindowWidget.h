#pragma once
#include "Circuit/Widgets/CompoundWidget.h"
#include "Circuit/Widgets/WindowTitlebarWidget.h"

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

		CIRCUIT_EVENT(OnTitlebarButtonInteracted, OnRequestClose);
		CIRCUIT_EVENT(OnTitlebarButtonInteracted, OnRequestMinimize);
		CIRCUIT_EVENT(OnTitlebarButtonInteracted, OnRequestMaximize);

		CIRCUIT_END_ARGS();

		virtual glm::vec2 GetDesiredSize() override { return { -1,-1 }; }
		void Build(const Arguments& args);
		virtual void OnPaint(CircuitPainter& painter) override;

		virtual bool IsHittestInvisible() const override { return false; };

		virtual bool IsHoveringTitlebar() const;
	private:

		Ref<Widget> m_content;
		Ref<WindowTitlebarWidget> m_titlebar;
	};

}
