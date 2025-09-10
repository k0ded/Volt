#pragma once

#include "RenderCore/TransientResourceSystem/ResourceViewCache.h"
#include "RenderCore/RenderGraph/RenderGraphRHIResource.h"

#include <RHIModule/Buffers/StorageBuffer.h>

namespace Volt
{
	class TransientBufferResource : public RGRHIBufferResource
	{
	public:
		TransientBufferResource(RefPtr<RHI::StorageBuffer> buffer, size_t hash, uint64_t framesToKeepAlive);
		~TransientBufferResource() override = default;

		VT_INLINE RefPtr<RHI::BufferView> GetOrCreateView(const RHI::BufferViewDesc& desc) override { return m_viewCache.GetOrCreateView(desc); }
		VT_INLINE RefPtr<RHI::StorageBuffer> GetRHIBuffer() const override { return m_buffer; }
		VT_INLINE size_t GetHash() const { return m_hash; }

		VT_INLINE bool TryAquire(uint64_t frameIndex) 
		{
			if (m_frameReleasedIndex + m_framesToKeepAlive <= frameIndex)
			{
				bool expected = false;
				return m_aquired.compare_exchange_weak(expected, true);
			}
			return false;
		}
		
		VT_INLINE void Release(uint64_t frameIndex) { m_frameReleasedIndex = frameIndex; m_aquired.store(false, std::memory_order::relaxed); }

	private:
		TransientBufferViewCache m_viewCache;
		RefPtr<RHI::StorageBuffer> m_buffer;
		size_t m_hash;
		uint64_t m_frameReleasedIndex;
		uint64_t m_framesToKeepAlive;

		std::atomic_bool m_aquired;
	};

	class TransientTextureResource : public RGRHITextureResource
	{
	public:
		TransientTextureResource(RefPtr<RHI::Image> image, size_t hash, uint64_t framesToKeepAlive);
		~TransientTextureResource() override = default;

		VT_INLINE RefPtr<RHI::ImageView> GetOrCreateView(const RHI::ImageViewDesc& desc) override { return m_viewCache.GetOrCreateView(desc); }
		VT_INLINE RefPtr<RHI::Image> GetRHITexture() const override { return m_image; }
		VT_INLINE size_t GetHash() const { return m_hash; }

		VT_INLINE bool TryAquire(uint64_t frameIndex)
		{
			if (m_frameReleasedIndex + m_framesToKeepAlive <= frameIndex)
			{
				bool expected = false;
				return m_aquired.compare_exchange_weak(expected, true);
			}
			return false;
		}

		VT_INLINE void Release(uint64_t frameIndex) { m_frameReleasedIndex = frameIndex; m_aquired.store(false, std::memory_order::relaxed); }

	private:
		TransientImageViewCache m_viewCache;
		RefPtr<RHI::Image> m_image;
		size_t m_hash;
		uint64_t m_frameReleasedIndex;
		uint64_t m_framesToKeepAlive;

		std::atomic_bool m_aquired;
	};

	class TransientUniformBufferResource : public RGRHIUniformBufferResource
	{
	public:
		TransientUniformBufferResource(RefPtr<RHI::UniformBuffer> uniformBuffer, size_t hash, uint64_t framesToKeepAlive);
		~TransientUniformBufferResource() override = default;

		RefPtr<RHI::BufferView> GetOrCreateView(const RHI::BufferViewDesc& desc) override { return m_viewCache.GetOrCreateView(desc); }
		RefPtr<RHI::UniformBuffer> GetRHIUniformBuffer() const override { return m_uniformBuffer; }
	
		VT_INLINE size_t GetHash() const { return m_hash; }

		VT_INLINE bool TryAquire(uint64_t frameIndex)
		{
			if (m_frameReleasedIndex + m_framesToKeepAlive <= frameIndex)
			{
				bool expected = false;
				return m_aquired.compare_exchange_weak(expected, true);
			}
			return false;
		}

		VT_INLINE void Release(uint64_t frameIndex) { m_frameReleasedIndex = frameIndex; m_aquired.store(false, std::memory_order::relaxed); }

	private:
		TransientUniformBufferViewCache m_viewCache;
		RefPtr<RHI::StorageBuffer> m_uniformBuffer;
		size_t m_hash;
		uint64_t m_frameReleasedIndex;
		uint64_t m_framesToKeepAlive;

		std::atomic_bool m_aquired;
	};
}
