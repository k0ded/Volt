#pragma once

#include "Sandbox/Window/EditorWindow.h"
#include "Sandbox/Utility/Helpers.h"

#include <Volt-Scene/Scene.h>
#include <EntitySystem/Entity.h>

#include <InputModule/Events/KeyboardEvents.h>



namespace Volt
{
	class Prefab;
}

class SceneViewPanel : public EditorWindow
{
public:
	SceneViewPanel(AssetReference<Volt::Scene>& scene, const String& id);
	void UpdateMainContent() override;

	void HighlightEntity(Volt::Entity entity);

private:
	bool OnKeyPressedEvent(Volt::KeyPressedEvent& e);

	void DrawSceneName();
	void DrawEntity(Volt::Entity entity, const String& filter);
	void CreatePrefabAndSetupEntities(Volt::Entity entity);
	void UpdatePrefabsInScene(Volt::Prefab& prefab, Volt::Entity srcEntity);

	void RebuildEntityDrawList();
	void RebuildEntityDrawListRecursive(Volt::Entity entityId, const String& filter);

	bool SearchRecursively(Volt::Entity id, const String& filter, uint32_t maxSearchDepth, uint32_t currentDepth = 0);
	bool SearchRecursivelyParent(Volt::Entity id, const String& filter, uint32_t maxSearchDepth, uint32_t currentDepth = 0);
	bool MatchesQuery(const String& text, const String& filter);
	bool HasComponent(Volt::Entity id, const String& filter);

	void DrawMainRightClickPopup();

	String m_searchQuery;
	bool m_hasSearchQuery = false;
	Volt::EntityID m_scrollToEntity = Volt::Entity::NullID();

	bool m_isRenamingLayer = false;
	uint32_t m_renamingLayer = 0;

	Vector<Volt::EntityID> m_entityDrawList;
	std::unordered_map<Volt::EntityID, ImGuiID> m_entityToImGuiID;
	bool m_rebuildDrawList = false;

	bool m_showEntityUUIDs = false;

	AssetReference<Volt::Scene>& m_scene;
};
