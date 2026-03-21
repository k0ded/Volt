#include "csbpch.h"
#include "InspectorWidget.h"

#include <Circuit/Widgets/BorderWidget.h>
#include <Circuit/Widgets/TextWidget.h>

InspectorWidget::InspectorWidget()
{}

InspectorWidget::~InspectorWidget()
{}

void InspectorWidget::Build(const Arguments& args)
{
	auto textWidget = CreateWidget(Circuit::TextWidget)
		.Text("Inspector")
		.Size(40.f)
		.Color(CircuitColor(0xffffffff));

	auto border = CreateWidget(Circuit::BorderWidget)
		.BackgroundColor(CircuitColor(35, 35, 35))
		.Padding({ 10.f, 10.f, 10.f, 10.f })
		.Content(textWidget);

	AddChildWidget(border);
}

glm::vec2 InspectorWidget::GetDesiredSize()
{
	return { -1, -1 };
}
