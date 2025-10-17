#include "vrpch.h"

#include "Volt-Renderer/Texture/EnvironmentTextureSerializer.h"
#include "Volt-Renderer/Texture/EnvironmentTexture.h"
#include "Volt-Renderer/Texture/TextureSerializer.h"

#include <AssetSystem/AssetManager.h>

namespace Volt
{
	struct EnvironmentTextureHeader
	{
		RHI::PixelFormat format;
		Vector<TextureSerializer::TextureMip> mips;
		uint32_t numLayers;

		static void Serialize(BinaryStreamWriter& streamWriter, const EnvironmentTextureHeader& data)
		{
			streamWriter.Write(data.format);
			streamWriter.Write(data.mips);
			streamWriter.Write(data.numLayers);
		}

		static void Deserialize(BinaryStreamReader& streamReader, EnvironmentTextureHeader& outData)
		{
			streamReader.Read(outData.format);
			streamReader.Read(outData.mips);
			streamReader.Read(outData.numLayers);
		}
	};

	void EnvironmentTextureSerializer::Serialize(const AssetMetadata& metadata, CustomAssetMetadataVector& customData, const Ref<Asset>& asset) const
	{
		Ref<EnvironmentTexture> environmentTexture = std::reinterpret_pointer_cast<EnvironmentTexture>(asset);

		RefPtr<RHI::Image> diffuseImage = environmentTexture->m_diffuseImage;
		RefPtr<RHI::Image> specularImage = environmentTexture->m_specularImage;

		EnvironmentTextureHeader diffuseHeader{};
		EnvironmentTextureHeader specularHeader{};
		diffuseHeader.format = diffuseImage->GetFormat();
		diffuseHeader.numLayers = diffuseImage->GetLayerCount();
		specularHeader.format = specularImage->GetFormat();
		specularHeader.numLayers = specularImage->GetLayerCount();

		Buffer diffuseImageBuffer = TextureSerializer::GetImageDataBuffer(diffuseImage, diffuseHeader.mips);
		Buffer specularImageBuffer = TextureSerializer::GetImageDataBuffer(specularImage, specularHeader.mips);

		BinaryStreamWriter streamWriter{};
		const size_t compressedDataOffset = AssetSerializer::WriteMetadata(metadata, asset->GetVersion(), streamWriter);

		streamWriter.Write(diffuseHeader);
		streamWriter.Write(specularHeader);
		streamWriter.Write(diffuseImageBuffer);
		streamWriter.Write(specularImageBuffer);

		diffuseImageBuffer.Release();
		specularImageBuffer.Release();

		const auto filePath = AssetManager::GetFilesystemPath(metadata.filePath);
		streamWriter.WriteToDisk(filePath, true, compressedDataOffset);
	}

	bool EnvironmentTextureSerializer::Deserialize(const AssetMetadata& metadata, Ref<Asset> destinationAsset) const
	{
		const auto filePath = AssetManager::GetFilesystemPath(metadata.filePath);

		if (!std::filesystem::exists(filePath))
		{
			VT_LOG(Error, "File {0} not found!", metadata.filePath);
			destinationAsset->SetFlag(AssetFlag::Missing, true);
			return false;
		}

		BinaryStreamReader streamReader{ filePath };

		if (!streamReader.IsStreamValid())
		{
			VT_LOG(Error, "Failed to open file: {0}!", metadata.filePath);
			destinationAsset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		SerializedAssetMetadata serializedMetadata = AssetSerializer::ReadMetadata(streamReader);
		VT_ASSERT_MSG(serializedMetadata.version == destinationAsset->GetVersion(), "Incompatible version!");

		EnvironmentTextureHeader diffuseHeader, specularHeader;
		Buffer diffuseImageBuffer, specularImageBuffer;

		streamReader.Read(diffuseHeader);
		streamReader.Read(specularHeader);
		streamReader.Read(diffuseImageBuffer);
		streamReader.Read(specularImageBuffer);

		RefPtr<RHI::Image> diffuseImage, specularImage;

		RHI::ImageDesc specification{};
		specification.format = diffuseHeader.format;
		specification.width = diffuseHeader.mips.front().width;
		specification.height = diffuseHeader.mips.front().height;
		specification.layers = diffuseHeader.numLayers;
		specification.mips = static_cast<uint32_t>(diffuseHeader.mips.size());
		specification.usage = RHI::ImageUsage::Texture;
		specification.generateMips = false;
		specification.isCubeMap = true;
		specification.debugName = filePath.stem().string();

		diffuseImage = RHI::Image::Create(specification);

		specification.format = specularHeader.format;
		specification.width = specularHeader.mips.front().width;
		specification.height = specularHeader.mips.front().height;
		specification.layers = specularHeader.numLayers;
		specification.mips = static_cast<uint32_t>(specularHeader.mips.size());

		specularImage = RHI::Image::Create(specification);

		TextureSerializer::UploadImageData(diffuseImage, diffuseHeader.format, diffuseHeader.mips, diffuseImageBuffer);
		TextureSerializer::UploadImageData(specularImage, specularHeader.format, specularHeader.mips, specularImageBuffer);

		Ref<EnvironmentTexture> environmentTexture = std::reinterpret_pointer_cast<EnvironmentTexture>(destinationAsset);
		environmentTexture->m_diffuseImage = diffuseImage;
		environmentTexture->m_specularImage = specularImage;

		diffuseImageBuffer.Release();
		specularImageBuffer.Release();

		return true;
	}
}
