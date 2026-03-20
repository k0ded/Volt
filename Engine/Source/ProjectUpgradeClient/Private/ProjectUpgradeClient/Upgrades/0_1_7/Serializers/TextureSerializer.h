#pragma once

#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializer.h"
#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializerRegistry.h"

#include <AssetSystem/AssetTypes.h>

#include <RHIModule/Images/Image.h>

#include <CoreUtilities/Buffer/DataBuffer.h>

namespace Volt
{
	class TextureSerializer : public AssetSerializer
	{
	public:
		struct TextureMip
		{
			uint32_t width;
			uint32_t height;
			size_t dataSize;
			size_t dataOffset;
		};

		void Serialize(const AssetMetadata_0_1_7* metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const override;
		bool Deserialize(const AssetMetadata_0_1_7* metadata, AssetReference<Asset> destinationAsset) const override;
	
		static DataBuffer GetImageDataBuffer(IntRef<RHI::Image> image, Vector<TextureMip>& outMips);
		static void UploadImageData(IntRef<RHI::Image> image, RHI::PixelFormat format, const Vector<TextureMip>& mips, const DataBuffer& dataBuffer);
	};
}
