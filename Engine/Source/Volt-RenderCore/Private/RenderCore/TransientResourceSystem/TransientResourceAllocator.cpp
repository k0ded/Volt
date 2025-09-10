#include "rcpch.h"
#include "RenderCore/TransientResourceSystem/TransientResourceAllocator.h"
#include "RenderCore/TransientResourceSystem/TransientResource.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/Swapchain.h>

#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/Math/Hash.h>
#include <CoreUtilities/EnumUtils.h>

namespace Volt
{
	VT_INLINE size_t GetBufferDescHash(const RHI::BufferDesc& desc)
	{
		size_t hash = Math::HashCombine(std::hash<uint32_t>()(desc.count), std::hash<uint64_t>()(desc.elementSize));
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

	VT_INLINE size_t GetUniformBufferDescHash(const RGUniformBufferDesc& desc)
	{
		size_t hash = Math::HashCombine(std::hash<uint32_t>()(desc.count), std::hash<uint64_t>()(desc.elementSize));
		return hash;
	}

	TransientResourceAllocator::TransientResourceAllocator()
	{
		VT_ENSURE(s_instance == nullptr);
		s_instance = this;

		m_transientBufferAllocator.AllocateArena(2048);
		m_transientTextureAllocator.AllocateArena(2048);
		m_transientUniformBufferAllocator.AllocateArena(2048);
	}

	TransientResourceAllocator::~TransientResourceAllocator()
	{
		s_instance = nullptr;
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
				if (transientBuffer->TryAquire(m_frameIndex))
				{
					return transientBuffer;
				}
			}
		}

		const bool isCpuAccessiable = EnumValueContainsFlag(desc.memoryUsage, RHI::MemoryUsage::CPUToGPU);
		
		RefPtr<RHI::GPUAllocator> allocator = isCpuAccessiable ? nullptr : RHI::GraphicsContext::GetTransientAllocator();

		RefPtr<RHI::StorageBuffer> rhiBbuffer = RHI::StorageBuffer::Create(desc, allocator);
		TransientBufferResourceRef transientBuffer = m_transientBufferAllocator.Allocate(rhiBbuffer, hash, isCpuAccessiable ? RHI::Swapchain::FramesInFlight : 1);
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
				if (transientTexture->TryAquire(m_frameIndex))
				{
					return transientTexture;
				}
			}
		}

		RHI::ImageDesc specification = desc;
		specification.initializeImage = false;

		const bool isCpuAccessiable = EnumValueContainsFlag(desc.memoryUsage, RHI::MemoryUsage::CPUToGPU);

		RefPtr<RHI::GPUAllocator> allocator = isCpuAccessiable ? nullptr : RHI::GraphicsContext::GetTransientAllocator();

		RefPtr<RHI::Image> rhiTexture = RHI::Image::Create(specification, nullptr, allocator);
		TransientTextureResourceRef transientTexture = m_transientTextureAllocator.Allocate(rhiTexture, hash, isCpuAccessiable ? RHI::Swapchain::FramesInFlight : 1);
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
				if (transientBuffer->TryAquire(m_frameIndex))
				{
					return transientBuffer;
				}
			}
		}

		RefPtr<RHI::UniformBuffer> rhiBbuffer = RHI::UniformBuffer::Create(static_cast<uint32_t>(desc.elementSize), nullptr, desc.count, desc.name);
		TransientUniformBufferResourceRef transientBuffer = m_transientUniformBufferAllocator.Allocate(rhiBbuffer, hash, RHI::Swapchain::FramesInFlight);
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
	}
}
