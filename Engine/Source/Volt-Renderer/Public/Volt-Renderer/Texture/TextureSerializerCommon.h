#pragma once

#include "Volt-Renderer/Config.h"

#include <RHIModule/Images/Image.h>

#include <CoreUtilities/Buffer/DataBuffer.h>

namespace Volt::TextureSerializerCommon
{
	struct TextureMip
	{
		uint32_t width;
		uint32_t height;
		size_t dataSize;
		size_t dataOffset;
	};

	VTR_API DataBuffer GetImageDataBuffer(RefPtr<RHI::Image> image, Vector<TextureMip>& outMips);
	VTR_API void UploadImageData(RefPtr<Volt::RHI::Image> image, Volt::RHI::PixelFormat format, const Vector<struct TextureMip>& mips, const DataBuffer& dataBuffer, bool waitForGPU = false);
}
