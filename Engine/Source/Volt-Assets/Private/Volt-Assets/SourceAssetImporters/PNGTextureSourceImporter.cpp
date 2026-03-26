#include "vtassetspch.h"
#include "Volt-Assets/SourceAssetImporters/PNGTextureSourceImporter.h"
#include "Volt-Assets/SourceAssetImporters/ImportConfigs.h"
#include "Volt-Assets/SourceAssetImporters/TextureCompression.h"
#include "Volt-Assets/SourceAssetImporters/TextureImportCommon.h"

#include <Volt-Renderer/Texture/Texture2D.h>
#include <Volt-Renderer/Utility/ImageUtility.h>

#include <AssetSystem/AssetManager.h>

#include <CoreUtilities/Profiling/Profiling.h>

#include <RHIModule/Images/Image.h>
#include <RHIModule/Images/ImageUtility.h>

#include <libpng/png.h>

#include <fstream>

VT_DEFINE_LOG_CATEGORY(LogPNGTextureSourceImporter);

namespace Volt
{
	//VT_REGISTER_SOURCE_ASSET_IMPORTER(({ ".png" }), PNGTextureSourceImporter);

	Vector<AssetReference<Asset>> PNGTextureSourceImporter::ImportInternal(const Filesystem::Path& filepath, const void* config, const SourceAssetUserImportData& userData) const
	{
		VT_PROFILE_FUNCTION();
		const TextureSourceImportConfig& importConfig = *reinterpret_cast<const TextureSourceImportConfig*>(config);
		FILE* filePtr;
		errno_t fileError = fopen_s(&filePtr, filepath.ToString().c_str(), "rb");
		VT_UNUSED(fileError);

		// #TODO_Ivar: Add error logging.
		if (!filePtr)
		{
			return {};
		}

		png_byte fileSignature[8];
		fread(fileSignature, 1, 8, filePtr);
		if (png_sig_cmp(fileSignature, 0, 8))
		{
			const String error = FormatString("Failed to import file {}! Reason: File is not a PNG!", filepath);
			VT_LOGC(Error, LogPNGTextureSourceImporter, error);
			userData.OnError(error);

			fclose(filePtr);
			return {};
		}

		// #TODO_Ivar: Add error logging.
		png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (!pngPtr)
		{
			fclose(filePtr);
			return {};
		}

		// #TODO_Ivar: Add error logging.
		png_infop infoPtr = png_create_info_struct(pngPtr);
		if (!infoPtr)
		{
			png_destroy_read_struct(&pngPtr, nullptr, nullptr);
			fclose(filePtr);
			return {};
		}

		// #TODO_Ivar: Add error logging.
		VT_DISABLE_WARNING(4611)
		if (setjmp(png_jmpbuf(pngPtr)))
		{
			png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
			fclose(filePtr);
			return {};
		}
		VT_RESTORE_WARNING()

		png_init_io(pngPtr, filePtr);
		png_set_sig_bytes(pngPtr, 8);

		png_read_info(pngPtr, infoPtr);

		const png_uint_32 width = png_get_image_width(pngPtr, infoPtr);
		const png_uint_32 height = png_get_image_height(pngPtr, infoPtr);
		const png_byte colorType = png_get_color_type(pngPtr, infoPtr);
		const png_byte bitDepth = png_get_bit_depth(pngPtr, infoPtr);
	
		int32_t intent;
		const bool isSRGB = png_get_sRGB(pngPtr, infoPtr, &intent);

		if (colorType == PNG_COLOR_TYPE_PALETTE)
		{
			png_set_palette_to_rgb(pngPtr);
		}

		if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
		{
			png_set_expand_gray_1_2_4_to_8(pngPtr);
		}

		if (!(colorType & PNG_COLOR_MASK_ALPHA))
		{
			png_set_add_alpha(pngPtr, 0xFF, PNG_FILLER_AFTER);
		}

		png_read_update_info(pngPtr, infoPtr);

		RHI::PixelFormat format = RHI::PixelFormat::R8G8B8A8_UNORM;

		if (bitDepth == 16)
		{
			format = RHI::PixelFormat::R16G16B16A16_SFLOAT;
		}
		else if (isSRGB)
		{
			format = RHI::PixelFormat::R8G8B8A8_SRGB;
		}

		png_size_t rowBytes = png_get_rowbytes(pngPtr, infoPtr);


		Vector<uint8_t> imageData;
		imageData.resize_uninitialized(rowBytes * height);

		Vector<uint8_t*> rowPtrs;
		rowPtrs.resize_uninitialized(height);

		for (png_uint_32 y = 0; y < height; ++y)
		{
			rowPtrs[y] = imageData.data() + y * rowBytes;
		}

		png_read_image(pngPtr, rowPtrs.data());
		png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
		fclose(filePtr);

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
				imageData.data(),
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
			image = RHI::Image::Create(specification, imageData.data());

			if (importConfig.generateMipMaps)
			{
				ImageUtility::GenerateMipMaps(image);
			}
		}

		AssetReference<Texture2D> voltTexture;

		if (importConfig.createAsMemoryAsset)
		{
			VT_CHECK_MSG(importConfig.targetAssetHandle == Asset::Null(), "Target asset handle for memory assets are not supported!");
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

		return { voltTexture };
	}

	SourceAssetFileInformation PNGTextureSourceImporter::GetSourceFileInformation(const Filesystem::Path& filepath) const
	{
		VT_ENSURE(false);
		return SourceAssetFileInformation();
	}
}
