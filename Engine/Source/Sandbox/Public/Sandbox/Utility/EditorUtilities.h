#pragma once

#include "Sandbox/Utility/AssetBrowserPopup.h"

#include <AssetSystem/AssetHandle.h>
#include <AssetSystem/AssetType.h>

#include <CoreUtilities/Filesystem/Path.h>

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
	static bool Property(const String& text, Volt::AssetHandle& assetHandle, AssetType wantedType = AssetTypes::None);
	static bool AssetBrowserPopupField(const String& id, Volt::AssetHandle& assetHandle, AssetType wantedType = AssetTypes::None);

	static bool SearchBar(String& outSearchQuery, bool& outHasSearchQuery, bool setAsActive = false);

	static SaveReturnState SaveFilePopup(const String& aId);

	static String GetDuplicatedNameFromEntity(const Volt::Entity& entity);

	static void MarkEntityAsEdited(const Volt::Scene& scene, const Volt::Entity& entity);
	static void MarkEntityAndChildrenAsEdited(const Volt::Scene& scene, const Volt::Entity& entity);

	static void MarkEntityComponentAsEdited(const Volt::Scene& scene, const Volt::Entity& entity, const VoltGUID& componentGUID);
	static void MarkEntityAndChildrenComponentAsEdited(const Volt::Scene& scene, const Volt::Entity& entity, const VoltGUID& componentGUID);

	static void DestroyEntity(Volt::Scene& scene, const Volt::Entity& entity);
	static void DestroyEntities(Volt::Scene& scene, const Vector<Volt::Entity>& entities);

	static bool IsAssetTypeFileExtension(AssetType assetType, const Filesystem::Path& filepath);

	static void IterateComponentsInEntity(const Volt::Entity& entity, std::function<void(const VoltGUID&)>&& func);

private:
	static bool AssetBrowserPopupInternal(const String& id, Volt::AssetHandle& assetHandle, bool startState, AssetType wantedType = AssetTypes::None);
	struct DefaultFalse
	{
		bool state = false;
	};

	inline static std::unordered_map<String, Ref<AssetBrowserPopup>> s_assetBrowserPopups;
	inline static std::unordered_map<String, DefaultFalse> s_assetBrowserPopupsOpen;

	EditorUtils() = delete;
};
