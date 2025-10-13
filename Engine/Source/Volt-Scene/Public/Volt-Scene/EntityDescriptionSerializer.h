#pragma once
#include "Volt-Scene/Config.h"

#include <AssetSystem/Serialization/AssetSerializer.h>

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
	class VTS_API EntityDescSerializer : public AssetSerializer
	{
	public:
		EntityDescSerializer();
		~EntityDescSerializer() override;

		void Serialize(const AssetMetadata& metadata, CustomAssetMetadataVector& customData, const Ref<Asset>& asset) const override;
		bool Deserialize(const AssetMetadata& metadata, Ref<Asset> destinationAsset) const override;

		void SerializeEntity(EntityID id, const Ref<Scene>& scene, YAMLMemoryStreamWriter& streamWriter) const;
		Entity DeserializeEntity(const Ref<Scene>& scene, YAMLMemoryStreamReader& streamReader) const;

		//if the entity and it's components have already been created, apply the data from the stream reader to them
		void DeserializeEntityInPlace(Volt::Entity entity, YAMLMemoryStreamReader& streamReader) const;


		static Vector<VoltGUID> FindComponentTypes(YAMLMemoryStreamReader& streamReader);

		static EntityDescSerializer& Get() { return *s_instance; }

		static std::filesystem::path GetSavePathForEntity_ThreadSafe(const Volt::AssetHandle& handle);

	private:
		inline static EntityDescSerializer* s_instance = nullptr;

		Entity CreateEntityFromUUIDThreadSafe(EntityID entityId, const Ref<Scene>& scene) const;

		void SerializeClass(const uint8_t* data, const size_t offset, const IComponentTypeDesc* compDesc, YAMLMemoryStreamWriter& streamWriter, bool isSubComponent) const;
		void SerializeArray(const uint8_t* data, const size_t offset, const IArrayTypeDesc* arrayDesc, YAMLMemoryStreamWriter& streamWriter) const;

		void DeserializeClass(uint8_t* data, const size_t offset, const IComponentTypeDesc* compDesc, Entity dstEntity, YAMLMemoryStreamReader& streamReader) const;
		void DeserializeArray(uint8_t* data, const size_t offset, const IArrayTypeDesc* arrayDesc, Entity dstEntity, YAMLMemoryStreamReader& streamReader) const;

		std::unordered_map<TypeTraits::TypeIndex, std::function<void(YAMLMemoryStreamWriter&, const uint8_t*, const size_t)>> m_typeSerializers;
		std::unordered_map<TypeTraits::TypeIndex, std::function<void(YAMLMemoryStreamReader&, uint8_t*, const size_t)>> m_typeDeserializers;
	};
}
