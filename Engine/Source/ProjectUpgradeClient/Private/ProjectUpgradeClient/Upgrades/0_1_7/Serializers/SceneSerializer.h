#pragma once

#include "Volt-Scene/Config.h"

#include <AssetSystem/AssetTypes.h>

#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializer.h"
#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializerRegistry.h"

class YAMLMemoryStreamWriter;
class YAMLMemoryStreamReader;

namespace Volt
{
	class Scene;
	struct WorldCell;

	class IArrayTypeDesc;
	class MonoScriptFieldCache;
	class IComponentTypeDesc;

	class SceneSerializer : public AssetSerializer
	{
	public:
		SceneSerializer();
		~SceneSerializer() override;

		void Serialize(const AssetMetadata_0_1_7* metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const override;
		bool Deserialize(const AssetMetadata_0_1_7* metadata, AssetReference<Asset> destinationAsset) const override;

		//void LoadWorldCell(const Ref<Scene>& scene, const WorldCell& worldCell) const;

		static SceneSerializer& Get() { return *s_instance; }

		inline static constexpr uint32_t ENTITY_MAGIC_VAL = 1515;

	private:
		inline static SceneSerializer* s_instance = nullptr;
		//todo: world engine
		//void SerializeWorldEngine(const Ref<Scene>& scene, YAMLMemoryStreamWriter& streamWriter) const;
		//void DeserializeWorldEngine(const Ref<Scene>& scene, YAMLMemoryStreamReader& streamReader) const;
	};
}
