#pragma once

#include <Circuit/Widgets/CompoundWidget.h>
#include <Circuit/Widgets/ListViewWidget.h>

#include <AssetSystem/AssetReference.h>

#include <EntitySystem/EntityID.h>

#include <Volt-Scene/Scene.h>

#include <string>

struct SceneViewTreeItem
{
	Volt::EntityID entityId;
	String name;
};

class SceneViewWidget : public Circuit::CompoundWidget
{
public:
	SceneViewWidget();
	virtual ~SceneViewWidget();

	CIRCUIT_BEGIN_ARGS(SceneViewWidget)
	{};

	CIRCUIT_ARGUMENT(AssetReference<Volt::Scene>, Scene);

	CIRCUIT_END_ARGS();

	void Build(const Arguments& args);

	virtual glm::vec2 GetDesiredSize() override;

private:
	Ref<Circuit::IListViewRow<SceneViewTreeItem>> GenerateEntityRow(SceneViewTreeItem& item);

	AssetReference<Volt::Scene> m_scene;
	Vector<SceneViewTreeItem> m_treeItems;
};
