#pragma once

#include "AssetSystem/Asset.h"

#include <functional>

namespace Volt
{
	class Asset;

	class VTAS_API AssetFactory
	{
	public:
		using AssetCreateFunction = std::function<Ref<Asset>()>;

		bool RegisterAssetType(VoltGUID typeGuid, const AssetCreateFunction& func);
		VT_NODISCARD Ref<Asset> CreateAssetOfType(AssetType type) const;

		static AssetFactory& Get();

	private:
		std::unordered_map<VoltGUID, AssetCreateFunction> m_assetFactoryFunctions;
	};

}

#define VT_REGISTER_ASSET_FACTORY(assetType, type) \
	inline static bool AssetFactory_ ## type ## _Registered = AssetFactory::Get().RegisterAssetType(assetType ## Type ##::guid, []() { return CreateRef<type>(); })
