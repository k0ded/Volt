#pragma once

#include <RHIModule/Images/ImageView.h>
#include <RHIModule/Images/Image.h>

#include <RHIModule/Buffers/BufferView.h>
#include <RHIModule/Buffers/StorageBuffer.h>

#include <CoreUtilities/Containers/Map.h>

namespace Volt
{
	class ResourceViewCache
	{
	public:
		ResourceViewCache() = default;
		ResourceViewCache(const ResourceViewCache& other);
		ResourceViewCache(ResourceViewCache&& other);

		ResourceViewCache& operator=(const ResourceViewCache& other);
		ResourceViewCache& operator=(ResourceViewCache&& other);

		RefPtr<RHI::BufferView> GetOrCreateBufferView(const RHI::BufferViewDesc& desc, RefPtr<RHI::StorageBuffer> rhiBuffer);
		RefPtr<RHI::ImageView> GetOrCreateImageView(const RHI::ImageViewDesc& desc, RefPtr<RHI::Image> rhiTexture);

	private:
		std::mutex m_bufferViewCacheMutex;
		std::mutex m_imageViewCacheMutex;

		Map<size_t, RefPtr<RHI::BufferView>> m_bufferViewCache;
		Map<size_t, RefPtr<RHI::ImageView>> m_imageViewCache;
	};
}
