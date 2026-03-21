#include "csbpch.h"
#include "AssetBrowserWidget.h"

#include <Circuit/Widgets/BorderWidget.h>
#include <Circuit/Widgets/TextWidget.h>

AssetBrowserWidget::AssetBrowserWidget()
{}

AssetBrowserWidget::~AssetBrowserWidget()
{}

void AssetBrowserWidget::Build(const Arguments& args)
{
	auto textWidget = CreateWidget(Circuit::TextWidget)
		.Text("Asset Browser")
		.Size(40.f)
		.Color(CircuitColor(0xffffffff));

	auto border = CreateWidget(Circuit::BorderWidget)
		.BackgroundColor(CircuitColor(30, 30, 30))
		.Padding({ 10.f, 10.f, 10.f, 10.f })
		.Content(textWidget);

	AddChildWidget(border);
}

glm::vec2 AssetBrowserWidget::GetDesiredSize()
{
	return { -1, -1 };
}
