#pragma once

#include <Volt-Renderer/Texture/TextureSerializerCommon.h>

#include <RHIModule/Core/RHICommon.h>

#include <CoreUtilities/Containers/ArrayView.h>

namespace Volt::TextureCompression
{
	void Compress(uint32_t width,
		uint32_t height,
		RHI::PixelFormat srcFormat,
		uint8_t* pixelData,
		RHI::PixelFormat dstFormat,
		bool generateMips,
		bool convertToSRGBIfRequired,
		DataBuffer& outPixelData,
		Vector<Volt::TextureSerializerCommon::TextureMip>& outTextureMips);
}
