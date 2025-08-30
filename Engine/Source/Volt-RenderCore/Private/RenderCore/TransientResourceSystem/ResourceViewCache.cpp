#include "rcpch.h"
#include "RenderCore/TransientResourceSystem/ResourceViewCache.h"

#include <CoreUtilities/Math/Hash.h>

namespace Volt
{
	ResourceViewCache::ResourceViewCache(const ResourceViewCache& other)
		: m_bufferViewCache(other.m_bufferViewCache),
		m_imageViewCache(other.m_imageViewCache)
	{}
	
	ResourceViewCache::ResourceViewCache(ResourceViewCache&& other)
		: m_bufferViewCache(std::move(other.m_bufferViewCache)),
		m_imageViewCache(std::move(other.m_imageViewCache))
	{}
	
	ResourceViewCache& ResourceViewCache::operator=(const ResourceViewCache& other)
	{
		m_bufferViewCache = other.m_bufferViewCache;
		m_imageViewCache = other.m_imageViewCache;
	
		return *this;
	}
	
	ResourceViewCache& ResourceViewCache::operator=(ResourceViewCache&& other)
	{
		m_bufferViewCache = std::move(other.m_bufferViewCache);
		m_imageViewCache = std::move(other.m_imageViewCache);

		return *this;
	}

	RefPtr<RHI::BufferView> ResourceViewCache::GetOrCreateBufferView(const RHI::BufferViewDesc& desc, RefPtr<RHI::StorageBuffer> rhiBuffer)
	{
		size_t hash = Math::HashCombine(std::hash<size_t>()(desc.offset), std::hash<size_t>()(desc.size));
		hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(desc.bufferFormat)));
		hash = Math::HashCombine(hash, std::hash<void*>()(rhiBuffer.GetRaw()));

		std::scoped_lock lock{ m_bufferViewCacheMutex };

		if (m_bufferViewCache.contains(hash))
		{
			return m_bufferViewCache.at(hash);
		}

		RefPtr<RHI::BufferView> view = rhiBuffer->GetView(desc);
		m_bufferViewCache[hash] = view;

		return view;
	}

	RefPtr<RHI::ImageView> ResourceViewCache::GetOrCreateImageView(const RHI::ImageViewDesc& desc, RefPtr<RHI::Image> rhiImage)
	{
		size_t hash = Math::HashCombine(std::hash<uint32_t>()(static_cast<uint32_t>(desc.viewType)), std::hash<uint32_t>()(desc.baseMipLevel));
		hash = Math::HashCombine(hash, std::hash<uint32_t>()(desc.baseArrayLayer));
		hash = Math::HashCombine(hash, std::hash<uint32_t>()(desc.mipCount));
		hash = Math::HashCombine(hash, std::hash<uint32_t>()(desc.layerCount));
		hash = Math::HashCombine(hash, std::hash<void*>()(rhiImage.GetRaw()));

		std::scoped_lock lock{ m_imageViewCacheMutex };

		if (m_imageViewCache.contains(hash))
		{
			return m_imageViewCache.at(hash);
		}

		RefPtr<RHI::ImageView> view = rhiImage->GetView(desc);
		m_imageViewCache[hash] = view;

		return view;
	}
}
