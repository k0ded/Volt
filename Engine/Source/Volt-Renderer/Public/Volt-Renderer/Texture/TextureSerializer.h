#pragma once

#include <AssetSystem/AssetTypes.h>

#include <AssetSystem/Serialization/AssetSerializer.h>
#include <AssetSystem/AssetSerializerRegistry.h>

#include <RHIModule/Images/Image.h>

#include <CoreUtilities/Buffer/Buffer.h>

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

		void Serialize(const AssetMetadata& metadata, CustomAssetMetadataVector& customData, const Ref<Asset>& asset) const override;
		bool Deserialize(const AssetMetadata& metadata, Ref<Asset> destinationAsset) const override;
	
		static Buffer GetImageDataBuffer(RefPtr<RHI::Image> image, Vector<TextureMip>& outMips);
		static void UploadImageData(RefPtr<RHI::Image> image, RHI::PixelFormat format, const Vector<TextureMip>& mips, const Buffer& dataBuffer);
	};
}
