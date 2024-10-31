#include "circuitpch.h"
#include "WindowWidget.h"

#include "Circuit/Widgets/Layout/LayoutWidget.h"
#include "Circuit/Widgets/ButtonWidget.h"
#include "TextWidget.h"

namespace Circuit
{
	void WindowWidget::Build(const Arguments& args)
	{
		m_content = args._Content;

		auto layout = CreateWidget(LayoutWidget).Orientation(LayoutOrientation::Vertical);

		
		layout->AddFixedSlice(BuildTitlebar(), 100);

		layout->AddFlexibleSlice(m_content);

		/*SetChildWidget(
			layout
		);*/
	}
	std::shared_ptr<LayoutWidget> WindowWidget::BuildTitlebar()
	{
		//Window titlebar
		auto titlebar = CreateWidget(LayoutWidget).Orientation(LayoutOrientation::Horizontal);

		titlebar->AddFlexibleSlice(CreateWidget(TextWidget)
			.Text("WINDOW TITLE!")
			.Size(21.f)
		);
		titlebar->AddFlexibleSlice(CreateWidget(ButtonWidget));
		titlebar->AddFlexibleSlice(CreateWidget(ButtonWidget));
		titlebar->AddFlexibleSlice(CreateWidget(ButtonWidget));

		return titlebar;
	}
}
