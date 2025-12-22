#pragma once

#include "Volt-Scene/Config.h"
#include "Volt-Scene/EntityUtility.h"
#include "Volt-Scene/Scene.h"

#include <AssetSystem/AssetTypes.h>

#include <EntitySystem/Entity.h>
#include <EntitySystem/Scripting/CoreComponents.h>

#include <AssetSystem/Asset.h>
#include <AssetSystem/AssetFactory.h>

namespace Volt
{
	class Scene;

	class VTS_API Prefab : public Asset
	{
	public:
		struct PrefabReferenceData
		{
			AssetHandle prefabAsset;
			EntityID prefabReferenceEntity;

			friend Archive& operator<<(Archive& archive, PrefabReferenceData& value)
			{
				archive << value.prefabAsset;
				archive << value.prefabReferenceEntity;

				return archive;
			}
		};

		Prefab() = default;
		Prefab(Entity srcRootEntity);
		Prefab(AssetReference<Scene> prefabScene, EntityID rootEntityId, uint32_t version);

		~Prefab() override = default;

		Entity Instantiate(Scene& targetScene);
		const bool UpdateEntityInPrefab(Entity srcEntity);
		void UpdateEntityInScene(Scene& targetScene, Entity sceneEntity);

		void CopyPrefabEntity(Entity dstEntity, EntityID srcPrefabEntityId, const std::set<VoltGUID> componentsToSkip = CreateSkipComponentOnCopySet<RelationshipComponent>()) const;

		[[nodiscard]] inline const bool IsPrefabValid() { return m_prefabScene.IsValid() && m_rootEntityId != Entity::NullID(); }
		[[nodiscard]] const bool IsEntityValidInPrefab(Entity entity) const;
		[[nodiscard]] const bool IsEntityValidInPrefab(EntityID prefabEntityId) const;
		[[nodiscard]] const bool IsEntityRoot(Entity entity) const;
		[[nodiscard]] const bool IsReference(Entity entity) const;

		[[nodiscard]] const PrefabReferenceData& GetReferenceData(Entity entity) const;

		uint32_t GetPrefabVersion() const { return m_version; }

		static AssetType GetStaticType() { return AssetTypes::Prefab; }
		AssetType GetType() const override { return GetStaticType(); };
		void Serialize(Archive& archive, ReadOnlyAssetMetadata assetMetadata) override;

	private:
		friend class PrefabImporter;

		[[nodiscard]] const Entity GetRootEntity() const;

		void InitializeComponents(Entity entity);

		void CreatePrefab(Entity srcRootEntity);
		void AddEntityToPrefabRecursive(Entity entity, Entity parentPrefabEntity);
		void ValidatePrefabUpdate(Entity srcEntity);
		void UpdatePrefabVersion(Entity entity, uint32_t targetVersion);

		const bool UpdateEntityInPrefabInternal(Entity srcEntity, EntityID rootSceneId, EntityID forcedPrefabEntity);
		void UpdateEntityInSceneInternal(Scene& scene, Entity sceneEntity, EntityID forcedPrefabEntity);

		Entity InstantiateEntity(Scene& scene, Entity prefabEntity);
		const Vector<Entity> FlattenEntityHeirarchy(Entity entity);

		AssetReference<Scene> m_prefabScene;
		Map<EntityID, PrefabReferenceData> m_prefabReferencesMap; // Maps this prefabs entity to an entity in another prefab

		EntityID m_rootEntityId = Entity::NullID();
		uint32_t m_version = 0;
	};
}
