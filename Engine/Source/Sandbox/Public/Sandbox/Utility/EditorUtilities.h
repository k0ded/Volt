#pragma once

#include "Sandbox/Utility/AssetBrowserPopup.h"

#include <AssetSystem/AssetHandle.h>
#include <AssetSystem/AssetType.h>

#include <filesystem>

namespace Volt
{
	class Texture2D;
	class Mesh;
	class Entity;
	class Scene;
}

enum class SaveReturnState
{
	None,
	Save,
	Discard
};

class EditorUtils
{
public:
	static bool Property(const std::string& text, Volt::AssetHandle& assetHandle, AssetType wantedType = AssetTypes::None);
	static bool AssetBrowserPopupField(const std::string& id, Volt::AssetHandle& assetHandle, AssetType wantedType = AssetTypes::None);

	static bool SearchBar(std::string& outSearchQuery, bool& outHasSearchQuery, bool setAsActive = false);

	static SaveReturnState SaveFilePopup(const std::string& aId);

	static std::string GetDuplicatedNameFromEntity(const Volt::Entity& entity);

	static void MarkEntityAsEdited(const Volt::Scene& scene, const Volt::Entity& entity);
	static void MarkEntityAndChildrenAsEdited(const Volt::Scene& scene, const Volt::Entity& entity);
	static void DestroyEntity(Volt::Scene& scene, const Volt::Entity& entity);
	static void DestroyEntities(Volt::Scene& scene, const Vector<Volt::Entity>& entities);

private:
	static bool AssetBrowserPopupInternal(const std::string& id, Volt::AssetHandle& assetHandle, bool startState, AssetType wantedType = AssetTypes::None);
	struct DefaultFalse
	{
		bool state = false;
	};

	inline static std::unordered_map<std::string, Ref<AssetBrowserPopup>> s_assetBrowserPopups;
	inline static std::unordered_map<std::string, DefaultFalse> s_assetBrowserPopupsOpen;

	EditorUtils() = delete;
};
