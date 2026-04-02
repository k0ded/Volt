#include "rcpch.h"
#include "RenderCore/TransientResourceSystem/TransientResourceAllocator.h"
#include "RenderCore/TransientResourceSystem/TransientResource.h"

#include <CoreUtilities/ConsoleVariableRegistry.h>

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/Swapchain.h>
#include <RHIModule/RHICapabilities.h>

#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/Math/Hash.h>
#include <CoreUtilities/EnumUtils.h>

namespace Volt
{
	static ConsoleVariable<int32_t> s_renderGraphTransientAllocatorNumFramesToKeepAliveResources(
		"r.RenderGraph.TransientAllocator.NumFramesToKeepAliveResources",
		4,
		""
	);

	VT_INLINE size_t GetBufferDescHash(const RHI::BufferDesc& desc)
	{
		size_t hash = Math::HashCombine(std::hash<uint64_t>()(desc.numElements), std::hash<uint64_t>()(desc.elementSize));
		hash = Math::HashCombine(hash, std::hash<uint16_t>()(static_cast<uint16_t>(desc.usage)));
		hash = Math::HashCombine(hash, std::hash<uint16_t>()(static_cast<uint8_t>(desc.memoryUsage)));

		return hash;
	}

	VT_INLINE size_t GetTextureDescHash(const RHI::ImageDesc& desc)
	{
		size_t hash = Math::HashCombine(std::hash<uint32_t>()(desc.width), std::hash<uint32_t>()(desc.height));
		hash = Math::HashCombine(hash, std::hash<uint32_t>()(desc.depth));
		hash = Math::HashCombine(hash, std::hash<uint32_t>()(desc.layers));
		hash = Math::HashCombine(hash, std::hash<uint32_t>()(desc.mips));
		hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(desc.format)));
		hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(desc.usage)));
		hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(desc.imageType)));
		hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(desc.memoryUsage)));

		return hash;
	}

	VT_INLINE size_t GetBufferDescHashWithRange(const RHI::BufferDesc& desc, const PagedAllocatedRange& range)
	{
		size_t hash = GetBufferDescHash(desc);
		hash = Math::HashCombine(hash, std::hash<uint64_t>()(range.size));
		hash = Math::HashCombine(hash, std::hash<uint64_t>()(range.offset));
		hash = Math::HashCombine(hash, std::hash<uint32_t>()(range.pageIndex));
	
		return hash;
	}

	VT_INLINE size_t GetTextureDescHashWithRange(const RHI::ImageDesc& desc, const PagedAllocatedRange& range)
	{
		size_t hash = GetTextureDescHash(desc);
		hash = Math::HashCombine(hash, std::hash<uint64_t>()(range.size));
		hash = Math::HashCombine(hash, std::hash<uint64_t>()(range.offset));
		hash = Math::HashCombine(hash, std::hash<uint32_t>()(range.pageIndex));

		return hash;
	}

	VT_INLINE size_t GetUniformBufferDescHash(const RGUniformBufferDesc& desc)
	{
		const size_t hash = std::hash<uint32_t>()(desc.size);
		return hash;
	}

	TransientResourceAllocator::TransientResourceAllocator()
	{
		VT_ENSURE(s_instance == nullptr);
		s_instance = this;

		m_transientBufferAllocator.ReservePages(1);
		m_transientTextureAllocator.ReservePages(1);
		m_transientUniformBufferAllocator.ReservePages(1);

		CreateHeaps();
	}

	TransientResourceAllocator::~TransientResourceAllocator()
	{
		m_bufferCache.clear();
		m_textureCache.clear();
		s_instance = nullptr;
	}

	TransientBufferResourceRef TransientResourceAllocator::CreateTransientBuffer(const RGBufferDesc& desc, const PagedAllocatedRange& requiredRange)
	{
		VT_PROFILE_FUNCTION();
		VT_MAYBE_UNUSED const bool isCpuAccessible = EnumValueContainsFlag(desc.memoryUsage, RHI::MemoryUsage::CPUToGPU);
		VT_ENSURE(!isCpuAccessible);

		const size_t hash = GetBufferDescHashWithRange(desc, requiredRange);

		for (size_t i = 0; i < m_bufferCache.size(); ++i)
		{
			TransientBufferResourceRef transientBuffer = m_bufferCache[i];

			if (transientBuffer->GetHash() == hash)
			{
				if (transientBuffer->TryAcquire(m_frameIndex))
				{
					return transientBuffer;
				}
			}
		}

		IntRef<RHI::TransientBuffer> rhiBbuffer = RHI::TransientBuffer::Create(desc);

		// Create the buffer and make sure we acquire it.
		TransientBufferResourceRef transientBuffer = m_transientBufferAllocator.Allocate(rhiBbuffer, hash, 1, true);
		transientBuffer->TryAcquire(m_frameIndex);

		// Bind the buffer to it's memory range
		rhiBbuffer->BindMemory(m_bufferHeap, requiredRange.pageIndex, requiredRange.offset);

		m_bufferCache.emplace_back(transientBuffer);

		return transientBuffer;
	}

	TransientTextureResourceRef TransientResourceAllocator::CreateTransientTexture(const RGTextureDesc& desc, const PagedAllocatedRange& requiredRange)
	{
		VT_PROFILE_FUNCTION();
		VT_MAYBE_UNUSED const bool isCpuAccessible = EnumValueContainsFlag(desc.memoryUsage, RHI::MemoryUsage::CPUToGPU);
		VT_ENSURE(!isCpuAccessible);

		const size_t hash = GetTextureDescHashWithRange(desc, requiredRange);

		for (size_t i = 0; i < m_textureCache.size(); ++i)
		{
			TransientTextureResourceRef transientTexture = m_textureCache[i];

			if (transientTexture->GetHash() == hash)
			{
				if (transientTexture->TryAcquire(m_frameIndex))
				{
					return transientTexture;
				}
			}
		}

		RHI::ImageDesc specification = desc;
		specification.initializeImage = false;

		IntRef<RHI::TransientImage> rhiTexture = RHI::TransientImage::Create(specification);

		// Create the texture and make sure we acquire it.
		TransientTextureResourceRef transientTexture = m_transientTextureAllocator.Allocate(rhiTexture, hash, 1, true);
		transientTexture->TryAcquire(m_frameIndex);

		// Bind the texture to it's memory range
		rhiTexture->BindMemory(m_textureHeap, requiredRange.pageIndex, requiredRange.offset);

		m_textureCache.emplace_back(transientTexture);

		return transientTexture;
	}

	TransientBufferResourceRef TransientResourceAllocator::CreateBuffer(const RGBufferDesc& desc)
	{
		VT_PROFILE_FUNCTION();
		const size_t hash = GetBufferDescHash(desc);

		for (size_t i = 0; i < m_bufferCache.size(); ++i)
		{
			TransientBufferResourceRef transientBuffer = m_bufferCache[i];

			if (transientBuffer->GetHash() == hash)
			{
				if (transientBuffer->TryAcquire(m_frameIndex))
				{
					return transientBuffer;
				}
			}
		}

		const bool isCpuAccessible = EnumValueContainsFlag(desc.memoryUsage, RHI::MemoryUsage::CPUToGPU);
		IntRef<RHI::Buffer> rhiBbuffer = RHI::Buffer::Create(desc);

		// Create the buffer and make sure we acquire it.
		TransientBufferResourceRef transientBuffer = m_transientBufferAllocator.Allocate(rhiBbuffer, hash, isCpuAccessible ? RHI::RHICapabilities::NumFramesInFlight : 1, false);
		transientBuffer->TryAcquire(m_frameIndex);

		m_bufferCache.emplace_back(transientBuffer);

		return transientBuffer;
	}

	void TransientResourceAllocator::FreeBuffer(TransientBufferResourceRef transientBuffer)
	{
		transientBuffer->Release(m_frameIndex);
	}

	TransientTextureResourceRef TransientResourceAllocator::CreateTexture(const RGTextureDesc& desc)
	{
		VT_PROFILE_FUNCTION();
		const size_t hash = GetTextureDescHash(desc);

		for (size_t i = 0; i < m_textureCache.size(); ++i)
		{
			TransientTextureResourceRef transientTexture = m_textureCache[i];

			if (transientTexture->GetHash() == hash)
			{
				if (transientTexture->TryAcquire(m_frameIndex))
				{
					return transientTexture;
				}
			}
		}

		RHI::ImageDesc specification = desc;
		specification.initializeImage = false;

		const bool isCpuAccessible = EnumValueContainsFlag(desc.memoryUsage, RHI::MemoryUsage::CPUToGPU);

		IntRef<RHI::Image> rhiTexture = RHI::Image::Create(specification);

		// Create the texture and make sure we acquire it.
		TransientTextureResourceRef transientTexture = m_transientTextureAllocator.Allocate(rhiTexture, hash, isCpuAccessible ? RHI::RHICapabilities::NumFramesInFlight : 1, false);
		transientTexture->TryAcquire(m_frameIndex);

		m_textureCache.emplace_back(transientTexture);

		return transientTexture;
	}

	void TransientResourceAllocator::FreeTexture(TransientTextureResourceRef transientTexture)
	{
		transientTexture->Release(m_frameIndex);
	}

	TransientUniformBufferResourceRef TransientResourceAllocator::CreateUniformBuffer(const RGUniformBufferDesc& desc)
	{
		VT_PROFILE_FUNCTION();
		const size_t hash = GetUniformBufferDescHash(desc);

		for (size_t i = 0; i < m_uniformBufferCache.size(); ++i)
		{
			TransientUniformBufferResourceRef transientBuffer = m_uniformBufferCache[i];

			if (transientBuffer->GetHash() == hash)
			{
				if (transientBuffer->TryAcquire(m_frameIndex))
				{
					return transientBuffer;
				}
			}
		}

		IntRef<RHI::UniformBuffer> rhiBbuffer = RHI::UniformBuffer::Create(desc);
		
		// Create the buffer and make sure we acquire it.
		TransientUniformBufferResourceRef transientBuffer = m_transientUniformBufferAllocator.Allocate(rhiBbuffer, hash, RHI::RHICapabilities::NumFramesInFlight);
		transientBuffer->TryAcquire(m_frameIndex);

		m_uniformBufferCache.emplace_back(transientBuffer);

		return transientBuffer;
	}

	void TransientResourceAllocator::FreeUniformBuffer(TransientUniformBufferResourceRef transientUniformBuffer)
	{
		transientUniformBuffer->Release(m_frameIndex);
	}

	void TransientResourceAllocator::OnPreRender(uint64_t frameIndex)
	{
		m_frameIndex = frameIndex;

		const uint64_t numFramesToKeepAliveResources = static_cast<uint64_t>(std::max(s_renderGraphTransientAllocatorNumFramesToKeepAliveResources.GetValue(), 0));

		for (int32_t i = static_cast<int32_t>(m_bufferCache.size() - 1); i >= 0; --i)
		{
			TransientBufferResourceRef buffer = m_bufferCache[i];
			if (!buffer->IsAcquired() && buffer->GetFrameReleased() + numFramesToKeepAliveResources <= m_frameIndex)
			{
				m_bufferCache.erase_unsorted(m_bufferCache.begin() + i);
				m_transientBufferAllocator.Free(buffer);
			}
		}

		for (int32_t i = static_cast<int32_t>(m_textureCache.size() - 1); i >= 0; --i)
		{
			TransientTextureResourceRef texture = m_textureCache[i];
			if (!texture->IsAcquired() && texture->GetFrameReleased() + numFramesToKeepAliveResources <= m_frameIndex)
			{
				m_textureCache.erase_unsorted(m_textureCache.begin() + i);
				m_transientTextureAllocator.Free(texture);
			}
		}

		for (int32_t i = static_cast<int32_t>(m_uniformBufferCache.size() - 1); i >= 0; --i)
		{
			TransientUniformBufferResourceRef buffer = m_uniformBufferCache[i];
			if (!buffer->IsAcquired() && buffer->GetFrameReleased() + numFramesToKeepAliveResources <= m_frameIndex)
			{
				m_uniformBufferCache.erase_unsorted(m_uniformBufferCache.begin() + i);
				m_transientUniformBufferAllocator.Free(buffer);
			}
		}
	}

	void TransientResourceAllocator::ReserveTexturePages(uint32_t numPages)
	{
		m_textureHeap->ReservePages(numPages);
	}

	void TransientResourceAllocator::ReserveBufferPages(uint32_t numPages)
	{
		m_bufferHeap->ReservePages(numPages);
	}

	uint64_t TransientResourceAllocator::GetPageSize() const
	{
		return 128 * 1024 * 1024;
	}

	void TransientResourceAllocator::CreateHeaps()
	{
		{
			RHI::TransientHeapCreateInfo createInfo;
			createInfo.alignment = 0;
			createInfo.pageSize = GetPageSize();
			createInfo.flags = RHI::TransientHeapFlags::AllowBuffers;

			m_bufferHeap = RHI::TransientHeap::Create(createInfo);
		}

		{
			RHI::TransientHeapCreateInfo createInfo{};
			createInfo.alignment = 0;
			createInfo.pageSize = GetPageSize();
			createInfo.flags = RHI::TransientHeapFlags::AllowTextures | RHI::TransientHeapFlags::AllowRenderTargets;
			
			m_textureHeap = RHI::TransientHeap::Create(createInfo);
		}
	}
}
