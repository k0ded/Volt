#include "vtassetspch.h"
#include "Volt-Assets/SourceAssetImporters/CommonTextureSourceImporter.h"
#include "Volt-Assets/SourceAssetImporters/ImportConfigs.h"
#include "Volt-Assets/SourceAssetImporters/TextureCompression.h"
#include "Volt-Assets/SourceAssetImporters/TextureImportCommon.h"

#include <Volt-Renderer/Texture/Texture2D.h>
#include <Volt-Renderer/Utility/ImageUtility.h>

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetReference.h>

#include <RHIModule/Images/Image.h>
#include <RHIModule/Images/ImageUtility.h>

#include <CoreUtilities/Profiling/Profiling.h>

#include <stb/stb_image.h>

VT_DEFINE_LOG_CATEGORY(LogCommonTextureSourceImporter);

namespace Volt
{
	VT_REGISTER_SOURCE_ASSET_IMPORTER(({ ".png", ".jpeg", ".jpg", ".tga", ".bmp", ".psd", ".gif", ".hdr", ".pic", ".pnm" }), CommonTextureSourceImporter);

	Vector<AssetReference<Asset>> CommonTextureSourceImporter::ImportInternal(const std::filesystem::path& filepath, const void* config, const SourceAssetUserImportData& userData) const
	{
		VT_PROFILE_FUNCTION();
		const TextureSourceImportConfig& importConfig = *reinterpret_cast<const TextureSourceImportConfig*>(config);

		int32_t width;
		int32_t height;
		int32_t channels;

		// #TODO_Ivar: Add failure checking
		stbi_set_flip_vertically_on_load(0);
		
		const bool isHDR = stbi_is_hdr(filepath.string().c_str());
		const bool is16Bit = stbi_is_16_bit(filepath.string().c_str());

		void* data = nullptr;

		if (isHDR)
		{
			data = stbi_loadf(filepath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
		}
		else
		{
			data = stbi_load(filepath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
		}

		if (!data)
		{
			const std::string error = std::format("Failed to import file {}. Reason: {}", filepath, stbi_failure_reason());
			VT_LOGC(Error, LogCommonTextureSourceImporter, error);
			userData.OnError(error);
			return {};
		}

		RHI::PixelFormat format = RHI::PixelFormat::R8G8B8A8_UNORM;

		if (is16Bit && isHDR)
		{
			format = isHDR ? RHI::PixelFormat::R16G16B16A16_SFLOAT : RHI::PixelFormat::R16G16B16A16_UNORM;
		}

		if (!is16Bit && isHDR)
		{
			format = RHI::PixelFormat::R32G32B32A32_SFLOAT;
		}

		const bool shouldCompressTexture = importConfig.compressionType != TextureCompressionType::None;
		uint32_t numMipMaps = 1;

		Vector<TextureSerializerCommon::TextureMip> compressedMips;
		DataBuffer compressedPixelData;
		if (shouldCompressTexture)
		{
			const RHI::PixelFormat dstFormat = TextureImport::GetFormatFromTextureCompressionType(importConfig.compressionType);

			TextureCompression::Compress(width,
				height,
				format,
				reinterpret_cast<uint8_t*>(data),
				dstFormat,
				true,
				true,
				compressedPixelData,
				compressedMips);

			format = dstFormat;
			numMipMaps = static_cast<uint32_t>(compressedMips.size());
		}
		else if (importConfig.generateMipMaps)
		{
			numMipMaps = RHI::Utility::CalculateMipCount(width, height);
		}

		RHI::ImageDesc specification{};
		specification.format = format;
		specification.usage = RHI::ImageUsage::Storage;
		specification.width = width;
		specification.height = height;
		specification.mips = numMipMaps;
		specification.debugName = importConfig.destinationFilename;

		IntRef<RHI::Image> image = nullptr;

		if (shouldCompressTexture)
		{
			image = RHI::Image::Create(specification);
			TextureSerializerCommon::UploadImageData(image, format, compressedMips, compressedPixelData, true);
		}
		else
		{
			image = RHI::Image::Create(specification, data);

			if (importConfig.generateMipMaps)
			{
				ImageUtility::GenerateMipMaps(image);
			}
		}

		AssetReference<Texture2D> voltTexture;

		if (importConfig.createAsMemoryAsset)
		{
			VT_CHECK_MSG(importConfig.targetAssetHandle == Asset::Null(), "Target asset handle with memory assets are not supported!");
			voltTexture = g_assetManager->CreateMemoryAsset<Texture2D>(importConfig.destinationFilename);
		}
		else
		{
			if (importConfig.targetAssetHandle != Asset::Null())
			{
				voltTexture = g_assetManager->CreateAssetWithAssetHandle<Texture2D>(importConfig.destinationFilename, importConfig.targetAssetHandle);
			}
			else
			{
				voltTexture = g_assetManager->CreateAsset<Texture2D>(importConfig.destinationFilename);
			}
		}

		voltTexture->SetImage(image);

		stbi_image_free(data);

		return { voltTexture };
	}

	SourceAssetFileInformation CommonTextureSourceImporter::GetSourceFileInformation(const std::filesystem::path& filepath) const
	{
		VT_ENSURE(false);
		return SourceAssetFileInformation();
	}
}
