#pragma once

#include <EntitySystem/EntityID.h>

#include <AssetSystem/AssetReference.h>

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Containers/Vector.h>

#include <unordered_map>

namespace Volt
{
	class Scene;
}

enum class SelectionContext
{
	Scene,

	GameUIEditor
};

class SelectionManager
{
public:
	using SelectionChangedCallback = std::function<void(const Vector<Volt::EntityID>&, SelectionContext)>;

	static void Initialize();
	static void Shutdown();

	static void RegisterSelectionChangedCallback(const SelectionChangedCallback& func);

	static bool Select(Volt::EntityID entity, SelectionContext context = SelectionContext::Scene);
	static bool Deselect(Volt::EntityID entity, SelectionContext context = SelectionContext::Scene);

	static void DeselectAll(SelectionContext context = SelectionContext::Scene);
	static bool IsAnySelected(SelectionContext context = SelectionContext::Scene);
	static bool IsSelected(Volt::EntityID entity, SelectionContext context = SelectionContext::Scene);

	static void Update(AssetReference<Volt::Scene> scene);

	static bool IsAnyParentSelected(Volt::EntityID entity, AssetReference<Volt::Scene> scene);

	inline static int32_t& GetFirstSelectedRow() { return m_firstSelectedRow; }
	inline static int32_t& GetLastSelectedRow() { return m_lastSelectedRow; }

	inline static const size_t GetSelectedCount(SelectionContext context = SelectionContext::Scene) { return m_entities[context].size(); }
	inline static const Vector<Volt::EntityID>& GetSelectedEntities(SelectionContext context = SelectionContext::Scene) { return m_entities[context]; }

	inline static void Lock() { m_locked = true; }
	inline static void Unlock() { m_locked = false; }

private:
	SelectionManager() = delete;

	inline static int32_t m_firstSelectedRow = -1;
	inline static int32_t m_lastSelectedRow = -1;
	inline static bool m_locked = false;
	inline static std::unordered_map<SelectionContext, Vector<Volt::EntityID>> m_entities;
	inline static Vector<SelectionChangedCallback> m_callbacks;
};
