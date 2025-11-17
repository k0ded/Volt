#pragma once

#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializer.h"

#include <CoreUtilities/TypeTraits/TypeIndex.h>

class YAMLMemoryStreamReader;
class YAMLMemoryStreamWriter;
namespace Volt
{
	class IArrayTypeDesc;
	class IComponentTypeDesc;
	class EntityID;
	class Scene;
	class Entity;
	class EntityDescSerializer : public AssetSerializer
	{
	public:
		EntityDescSerializer();
		~EntityDescSerializer() override;

		void Serialize(ReadOnlyAssetMetadata metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const override;
		bool Deserialize(ReadOnlyAssetMetadata metadata, AssetReference<Asset> destinationAsset) const override;

		Entity DeserializeEntity(AssetReference<Scene> scene, YAMLMemoryStreamReader& streamReader) const;

		static Vector<VoltGUID> FindComponentTypes(YAMLMemoryStreamReader& streamReader);

		static EntityDescSerializer& Get() { return *s_instance; }

		static std::filesystem::path GetSavePathForEntity_ThreadSafe(const Volt::AssetHandle& handle);

	private:
		inline static EntityDescSerializer* s_instance = nullptr;

		Entity CreateEntityFromUUIDThreadSafe(EntityID entityId, AssetReference<Scene> scene) const;

		void DeserializeClass(uint8_t* data, const size_t offset, const IComponentTypeDesc* compDesc, Entity dstEntity, YAMLMemoryStreamReader& streamReader) const;
		void DeserializeArray(uint8_t* data, const size_t offset, const IArrayTypeDesc* arrayDesc, Entity dstEntity, YAMLMemoryStreamReader& streamReader) const;

		std::unordered_map<TypeTraits::TypeIndex, std::function<void(YAMLMemoryStreamWriter&, const uint8_t*, const size_t)>> m_typeSerializers;
		std::unordered_map<TypeTraits::TypeIndex, std::function<void(YAMLMemoryStreamReader&, uint8_t*, const size_t)>> m_typeDeserializers;
	};
}
