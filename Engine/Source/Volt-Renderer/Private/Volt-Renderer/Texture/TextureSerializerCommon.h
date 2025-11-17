#pragma once

#include <RHIModule/Images/Image.h>

#include <CoreUtilities/Buffer/Buffer.h>

namespace Volt::TextureSerializerCommon
{
	struct TextureMip
	{
		uint32_t width;
		uint32_t height;
		size_t dataSize;
		size_t dataOffset;
	};

	Buffer GetImageDataBuffer(RefPtr<RHI::Image> image, Vector<TextureMip>& outMips);
	void UploadImageData(RefPtr<Volt::RHI::Image> image, Volt::RHI::PixelFormat format, const Vector<struct TextureMip>& mips, const Buffer& dataBuffer);
}
