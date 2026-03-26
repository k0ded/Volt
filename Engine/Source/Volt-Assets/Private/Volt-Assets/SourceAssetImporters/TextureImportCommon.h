#pragma once

#include "Volt-Assets/SourceAssetImporters/ImportConfigs.h"

#include <RHIModule/Core/RHICommon.h>

#include <string>

namespace Volt::TextureImport
{
	inline RHI::PixelFormat GetFormatFromTextureCompressionType(TextureCompressionType compressionType)
	{
		switch (compressionType)
		{
			case TextureCompressionType::BC7: return RHI::PixelFormat::BC7_SRGB_BLOCK;
			case TextureCompressionType::BC5: return RHI::PixelFormat::BC5_UNORM_BLOCK;
			case TextureCompressionType::BC3: return RHI::PixelFormat::BC3_UNORM_BLOCK;
			case TextureCompressionType::BC1: return RHI::PixelFormat::BC1_RGBA_UNORM_BLOCK;
		}

		VT_ENSURE(false);
		return RHI::PixelFormat::UNDEFINED;
	}

	TextureCompressionType TryGetTextureCompressionTypeFromFilename(const String& filename);
}
