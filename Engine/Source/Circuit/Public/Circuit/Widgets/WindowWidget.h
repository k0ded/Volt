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

		void Build(const Arguments& args);
		
	private:
		Ref<LayoutWidget> BuildTitlebar();
		Ref<Widget> m_content;
	};

}
