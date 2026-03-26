#include "csbpch.h"
#include "SceneViewWidget.h"

#include <Circuit/Widgets/BorderWidget.h>
#include <Circuit/Widgets/TextWidget.h>
#include <Circuit/Widgets/ListViewWidget.h>
#include <Circuit/Widgets/Layout/LayoutWidget.h>

SceneViewWidget::SceneViewWidget()
{}

SceneViewWidget::~SceneViewWidget()
{}

void SceneViewWidget::Build(const Arguments& args)
{
	m_scene = args._Scene;

	if (m_scene.IsValid())
	{
		Vector<Volt::Entity> entities = m_scene->GetAllEntities();
		for (const auto& entity : entities)
		{
			SceneViewTreeItem& item = m_treeItems.emplace_back();
			item.entityId = entity.GetID();
			item.name = entity.GetTag();
		}
	}

	auto titleWidget = CreateWidget(Circuit::TextWidget)
		.Text("Scene View")
		.Size(50.f)
		.Color(CircuitColor(0xffffffff));

	auto entityList = CreateWidget(Circuit::ListViewWidget<SceneViewTreeItem>)
		.ItemsSource(&m_treeItems)
		.OnGenerateRow_Raw(this, &SceneViewWidget::GenerateEntityRow);

	auto layout = CreateWidget(Circuit::LayoutWidget)
		.Orientation(Circuit::LayoutOrientation::Vertical);
	layout->AddFixedSlice(titleWidget, 60.f);
	layout->AddFlexibleSlice(entityList);

	auto border = CreateWidget(Circuit::BorderWidget)
		.BackgroundColor(CircuitColor(40, 40, 40))
		.Padding({ 10.f, 10.f, 10.f, 10.f })
		.Content(layout);

	AddChildWidget(border);
}

glm::vec2 SceneViewWidget::GetDesiredSize()
{
	return { -1, -1 };
}

Ref<Circuit::IListViewRow<SceneViewTreeItem>> SceneViewWidget::GenerateEntityRow(SceneViewTreeItem& item)
{
	auto textWidget = CreateWidget(Circuit::TextWidget)
		.Text(item.name)
		.Size(20.f)
		.Color(CircuitColor(0xffffffff));

	auto row = CreateWidget(Circuit::IListViewRow<SceneViewTreeItem>)
		.Content(textWidget);

	return row;
}
