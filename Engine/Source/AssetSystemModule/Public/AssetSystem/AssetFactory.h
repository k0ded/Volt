#pragma once

#include "AssetSystem/AssetManagerCommon.h"
#include "AssetSystem/AssetAllocatorCommon.h"

#include <functional>

namespace Volt
{
	class Asset;

	class VTAS_API AssetFactory
	{
	public:
		using CreateAssetTypeAllocatorFunction = std::function<Ref<AssetTypeAllocator>()>;

		struct FactoryData
		{
			CreateAssetTypeAllocatorFunction createAllocatorFunction;
			uint64_t assetTypeSize;
		};

		template<typename T> bool RegisterAssetType(VoltGUID typeGuid);
		VT_NODISCARD VT_INLINE const Map<VoltGUID, FactoryData>& GetFactoryMap() const { return m_assetFactoryFunctions; }

		static AssetFactory& Get();

	private:
		Map<VoltGUID, FactoryData> m_assetFactoryFunctions;
	};

	template<typename T>
	bool AssetFactory::RegisterAssetType(VoltGUID typeGuid)
	{
		FactoryData& factoryData = m_assetFactoryFunctions[typeGuid];
		factoryData.createAllocatorFunction = []() { return CreateRef<AssetTypeAllocatorImpl<T>>(); };
		factoryData.assetTypeSize = sizeof(T);

		return true;
	}
}

#define VT_REGISTER_ASSET_FACTORY(assetType, type) \
	inline static bool AssetFactory_ ## type ## _Registered = AssetFactory::Get().RegisterAssetType<type>(assetType ## Type::guid)
