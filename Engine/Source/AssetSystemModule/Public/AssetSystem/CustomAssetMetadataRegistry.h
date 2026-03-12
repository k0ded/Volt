#pragma once

#include "AssetSystem/Config.h"
#include "AssetSystem/AssetType.h"

#include <CoreUtilities/VoltGUID.h>
#include <CoreUtilities/Any.h>
#include <CoreUtilities/Archive/Archive.h>

namespace Volt
{
	class CustomAssetMetadata;
	class CustomAssetMetadataRegistry
	{
	public:
		template<typename T>
		void RegisterCustomMetadata(AssetType assetType);

		VTAS_API void UnregisterCustomMetadata(AssetType assetType);

		VTAS_API bool AssetTypeHasCustomMetadata(AssetType assetType) const;
		VTAS_API void SerializeAny(AssetType assetType, Any& value, Archive& archive) const;
		VTAS_API void SetupInitalCustomMetadata(AssetType assetType, CustomAssetMetadata& customMetadata) const;

		VTAS_API static CustomAssetMetadataRegistry& Get();

	private:
		struct RegisteredCustomMetadata
		{
			AssetType assetType;

			void(*SerializeAny)(Any&, Archive&);
			void(*InitializeAny)(Any&);
		};

		Map<AssetType, RegisteredCustomMetadata> m_registry;
	};

	template<typename T> 
	void CustomAssetMetadataRegistry::RegisterCustomMetadata(AssetType assetType)
	{
		RegisteredCustomMetadata& registeredData = m_registry[assetType];
		registeredData.assetType = assetType;
		registeredData.SerializeAny = [](Any& any, Archive& archive)
		{
			// Make sure the any is initialized to the correct value.
			if (archive.IsLoading())
			{
				any = Any(T());
			}

			T& data = any.Cast<T>();
			archive << data;
		};

		registeredData.InitializeAny = [](Any& any)
		{
			any = Any(T());
		};
	}
}

// Must lie in a compilation unit (cpp file)
#define VT_REGISTER_CUSTOM_ASSET_METADATA_TYPE(type, assetType) \
	class CustomAssetMetadataRegistrar_##type \
	{ \
	public: \
		VT_INLINE CustomAssetMetadataRegistrar_##type() \
		{ \
			CustomAssetMetadataRegistry::Get().RegisterCustomMetadata<type>(assetType); \
		} \
		VT_INLINE ~CustomAssetMetadataRegistrar_##type() \
		{ \
			CustomAssetMetadataRegistry::Get().UnregisterCustomMetadata(assetType); \
		} \
	} g_customAssetMetadataRegistrar_##type
