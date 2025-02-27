#pragma once

#include "RenderCore/Config.h"

#include <Volt-Core/AssetTypes.h>

#include <AssetSystem/Asset.h>
#include <AssetSystem/AssetFactory.h>

namespace Volt
{
	class ShaderSourceAsset : public Asset
	{
	public:
		~ShaderSourceAsset() override = default;

		static AssetType GetStaticType() { return AssetTypes::ShaderSource; }
		VT_INLINE AssetType GetType() override { return GetStaticType(); }
		VT_INLINE uint32_t GetVersion() const override { return 1; }

		inline static constexpr std::string_view Extension = ".hlsl";
		inline static constexpr std::string_view ExtensionInclude = ".hlsli";

	private:
	};

	VT_REGISTER_ASSET_FACTORY(AssetTypes::ShaderSource, ShaderSourceAsset);
}
