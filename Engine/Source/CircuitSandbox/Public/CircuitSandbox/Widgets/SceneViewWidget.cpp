#include "csbpch.h"
#include "SceneViewWidget.h"

#include <Circuit/Widgets/BorderWidget.h>
#include <Circuit/Widgets/TextWidget.h>

SceneViewWidget::SceneViewWidget()
{}

SceneViewWidget::~SceneViewWidget()
{}

void SceneViewWidget::Build(const Arguments& args)
{
	auto textWidget = CreateWidget(Circuit::TextWidget)
		.Text("Scene View")
		.Size(50.f)
		.Color(CircuitColor(0xffffffff));

	auto border = CreateWidget(Circuit::BorderWidget)
		.BackgroundColor(CircuitColor(40, 40, 40))
		.Padding({ 10.f, 10.f, 10.f, 10.f })
		.Content(textWidget);

	AddChildWidget(border);
}

glm::vec2 SceneViewWidget::GetDesiredSize()
{
	return { -1, -1 };
}
