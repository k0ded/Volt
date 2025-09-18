#include "rcpch.h"
#include "RenderCore/TransientResourceSystem/ResourceViewCache.h"
#include "RenderCore/TransientResourceSystem/TransientResource.h"

#include <RHIModule/Buffers/BufferView.h>

#include <CoreUtilities/Math/Hash.h>
#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	namespace Utility
	{
		VT_INLINE size_t GetHashFromBufferViewDesc(const RHI::BufferViewDesc& desc)
		{
			size_t hash = Math::HashCombine(std::hash<size_t>()(desc.offset), std::hash<size_t>()(desc.size));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(desc.bufferFormat)));
			return hash;
		}

		VT_INLINE size_t GetHashFromImageViewDesc(const RHI::ImageViewDesc& desc)
		{
			size_t hash = Math::HashCombine(std::hash<uint32_t>()(static_cast<uint32_t>(desc.viewType)), std::hash<uint32_t>()(desc.baseMipLevel));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(desc.baseArrayLayer));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(desc.mipCount));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(desc.layerCount));

			return hash;
		}
	}

	TransientBufferViewCache::TransientBufferViewCache(RGRHIBufferResource* buffer)
		: m_buffer(buffer)
	{}

	RefPtr<Volt::RHI::BufferView> TransientBufferViewCache::GetOrCreateView(const RHI::BufferViewDesc& desc)
	{
		VT_PROFILE_FUNCTION();

		const size_t hash = Utility::GetHashFromBufferViewDesc(desc);

		for (size_t i = 0; i < m_views.size(); ++i)
		{
			if (m_views[i].hash == hash)
			{
				return m_views[i].view;
			}
		}

		RefPtr<RHI::BufferView> bufferView = m_buffer->GetRHIBuffer()->GetView(desc);
		m_views.emplace_back(hash, bufferView);

		return bufferView;
	}

	TransientImageViewCache::TransientImageViewCache(RGRHITextureResource* texture)
		: m_texture(texture)
	{}

	RefPtr<RHI::ImageView> TransientImageViewCache::GetOrCreateView(const RHI::ImageViewDesc& desc)
	{
		VT_PROFILE_FUNCTION();
		
		const size_t hash = Utility::GetHashFromImageViewDesc(desc);

		for (size_t i = 0; i < m_views.size(); ++i)
		{
			if (m_views[i].hash == hash)
			{
				return m_views[i].view;
			}
		}

		RefPtr<RHI::ImageView> imageView = m_texture->GetRHITexture()->GetView(desc);
		m_views.emplace_back(hash, imageView);

		return imageView;
	}

	TransientUniformBufferViewCache::TransientUniformBufferViewCache(RGRHIUniformBufferResource* buffer)
		: m_buffer(buffer)
	{
	}

	RefPtr<Volt::RHI::BufferView> TransientUniformBufferViewCache::GetOrCreateView(const RHI::BufferViewDesc& desc)
	{
		VT_PROFILE_FUNCTION();

		const size_t hash = Utility::GetHashFromBufferViewDesc(desc);

		for (size_t i = 0; i < m_views.size(); ++i)
		{
			if (m_views[i].hash == hash)
			{
				return m_views[i].view;
			}
		}

		RefPtr<RHI::BufferView> bufferView = m_buffer->GetRHIUniformBuffer()->GetView(desc);
		m_views.emplace_back(hash, bufferView);

		return bufferView;
	}
}
