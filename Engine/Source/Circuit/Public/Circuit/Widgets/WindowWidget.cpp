#include "circuitpch.h"
#include "WindowWidget.h"

#include "Circuit/Widgets/Layout/LayoutWidget.h"
#include "Circuit/Widgets/ButtonWidget.h"
#include "TextWidget.h"

#include "Circuit/CircuitPainter.h"

namespace Circuit
{
	void WindowWidget::Build(const Arguments& args)
	{
		m_content = args._Content;

		auto layout = CreateWidget(LayoutWidget).Orientation(LayoutOrientation::Vertical);


		layout->AddFixedSlice(BuildTitlebar(), 100);

		if (m_content)
		{
			layout->AddFlexibleSlice(m_content);
		}

		AddChildWidget(layout);
	}

	std::shared_ptr<LayoutWidget> WindowWidget::BuildTitlebar()
	{
		//Window titlebar
		auto titlebar = CreateWidget(LayoutWidget).Orientation(LayoutOrientation::Horizontal);

		//titlebar->AddFlexibleSlice(CreateWidget(TextWidget)
		//	.Text("WINDOW TITLE!")
		//	.Size(21.f)
		//);

		//titlebar->AddFixedSlice(CreateWidget(TextWidget).Text(" "), 10);
		//titlebar->AddFlexibleSlice(CreateWidget(ButtonWidget).Content(CreateWidget(TextWidget).Text("_").Size(30)));
		//titlebar->AddFixedSlice(CreateWidget(TextWidget).Text(" "), 10);
		//titlebar->AddFlexibleSlice(CreateWidget(ButtonWidget).Content(CreateWidget(TextWidget).Text("O").Size(30)));
		//titlebar->AddFixedSlice(CreateWidget(TextWidget).Text(" "), 10);
		//titlebar->AddFlexibleSlice(CreateWidget(ButtonWidget).Content(CreateWidget(TextWidget).Text("X").Size(30)));

		return titlebar;
	}
}
