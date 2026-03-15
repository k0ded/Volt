#include "circuitpch.h"
#include "WindowWidget.h"

#include "Circuit/Widgets/Layout/LayoutWidget.h"
#include "Circuit/Widgets/ButtonWidget.h"
#include "TextWidget.h"

#include "Circuit/CircuitPainter.h"

namespace Circuit
{
	//void WindowWidget::OnLayout(const glm::vec2& allotedSize)
	//{
	//	VT_ASSERT_MSG(allotedSize.x != -1 && allotedSize.y != -1, "WindowWidget cannot be given adaptive size");

	//	for (Ref<Widget> child : GetChildren())
	//	{
	//		child->OnLayout(allotedSize);
	//	}

	//	return allotedSize;
	//}
	void WindowWidget::Build(const Arguments& args)
	{
		m_content = args._Content;

		auto layout = CreateWidget(LayoutWidget).Orientation(LayoutOrientation::Vertical);


		layout->AddFixedSlice(BuildTitlebar(), 30);

		if (m_content)
		{
			layout->AddFlexibleSlice(m_content);
		}

		AddChildWidget(layout);
	}

	void WindowWidget::OnPaint(CircuitPainter& painter)
	{
		const CircuitColor windowBgColor(59, 59, 59);

		painter.AddRect(0, 0, painter.GetAllotedSize().x, painter.GetAllotedSize().y, windowBgColor);

		CompoundWidget::OnPaint(painter);
	}

	std::shared_ptr<LayoutWidget> WindowWidget::BuildTitlebar()
	{
		//Window titlebar
		auto titlebar = CreateWidget(LayoutWidget).Orientation(LayoutOrientation::Horizontal);

		titlebar->AddFlexibleSlice(CreateWidget(ButtonWidget).MinSize(30), 2);
		titlebar->AddFixedSlice(CreateWidget(ButtonWidget), 30, 2);
		titlebar->AddFixedSlice(CreateWidget(ButtonWidget), 30, 2);
		titlebar->AddFixedSlice(CreateWidget(ButtonWidget), 30, 2);

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
