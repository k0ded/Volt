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

		void Serialize(const AssetMetadata& metadata, StackVector<uint8_t, ASSET_METADATA_SIZE>& customData, const Ref<Asset>& asset) const override;
		bool Deserialize(const AssetMetadata& metadata, Ref<Asset> destinationAsset) const override;

		void SerializeEntity(EntityID id, const Ref<Scene>& scene, YAMLMemoryStreamWriter& streamWriter) const;
		Entity DeserializeEntity(const Ref<Scene>& scene, YAMLMemoryStreamReader& streamReader) const;

		static EntityDescSerializer& Get() { return *s_instance; }

	private:
		inline static EntityDescSerializer* s_instance = nullptr;

		Entity CreateEntityFromUUIDThreadSafe(EntityID entityId, const Ref<Scene>& scene) const;

		void SerializeClass(const uint8_t* data, const size_t offset, const IComponentTypeDesc* compDesc, YAMLMemoryStreamWriter& streamWriter, bool isSubComponent) const;
		void SerializeArray(const uint8_t* data, const size_t offset, const IArrayTypeDesc* arrayDesc, YAMLMemoryStreamWriter& streamWriter) const;

		void DeserializeClass(uint8_t* data, const size_t offset, const IComponentTypeDesc* compDesc, Entity dstEntity, YAMLMemoryStreamReader& streamReader) const;
		void DeserializeArray(uint8_t* data, const size_t offset, const IArrayTypeDesc* arrayDesc, Entity dstEntity, YAMLMemoryStreamReader& streamReader) const;

		inline static std::unordered_map<TypeTraits::TypeIndex, std::function<void(YAMLMemoryStreamWriter&, const uint8_t*, const size_t)>> s_typeSerializers;
		inline static std::unordered_map<TypeTraits::TypeIndex, std::function<void(YAMLMemoryStreamReader&, uint8_t*, const size_t)>> s_typeDeserializers;
	};
}
