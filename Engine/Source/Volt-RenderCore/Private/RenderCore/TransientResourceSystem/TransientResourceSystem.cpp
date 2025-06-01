#include "rcpch.h"

#include "RenderCore/TransientResourceSystem/TransientResourceSystem.h"

#include <RHIModule/Images/Image.h>
#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Buffers/UniformBuffer.h>
#include <RHIModule/Graphics/GraphicsContext.h>

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	TransientResourceSystem::TransientResourceSystem()
	{
	}
	TransientResourceSystem::~TransientResourceSystem()
	{
		std::scoped_lock lock{ m_allocatedResourcesMutex };
		m_allocatedResources.clear();
	}
	
	TransientResourceSystem::TransientResourceSystem(const TransientResourceSystem& other) noexcept
	{
		m_allocatedResources = other.m_allocatedResources;
		m_surrenderedResources = other.m_surrenderedResources;
	}
	
	TransientResourceSystem::TransientResourceSystem(TransientResourceSystem&& other) noexcept
	{
		m_allocatedResources = std::move(other.m_allocatedResources);
		m_surrenderedResources = std::move(other.m_surrenderedResources);
	}

	TransientResourceSystem& TransientResourceSystem::operator=(const TransientResourceSystem& other) noexcept
	{
		m_allocatedResources = other.m_allocatedResources;
		m_surrenderedResources = other.m_surrenderedResources;

		return *this;
	}
	
	TransientResourceSystem& TransientResourceSystem::operator=(TransientResourceSystem&& other) noexcept
	{
		m_allocatedResources = std::move(other.m_allocatedResources);
		m_surrenderedResources = std::move(other.m_surrenderedResources);

		return *this;
	}
	
	RefPtr<RHI::Image> TransientResourceSystem::AcquireTexture(RGTextureRef resource)
	{
		VT_PROFILE_FUNCTION();

		{
			std::scoped_lock lock{ m_allocatedResourcesMutex };
			if (m_allocatedResources.contains(resource))
			{
				return m_allocatedResources.at(resource).resource.As<RHI::Image>();
			}
		}

		RefPtr<RHI::Image> image = RHI::Image::Create(resource->GetDesc(), nullptr, RHI::GraphicsContext::GetTransientAllocator());

		ResourceInfo info{};
		info.resource = image;
		info.isOriginal = true;

		{
			std::scoped_lock lock{ m_allocatedResourcesMutex };
			m_allocatedResources[resource] = info;
		}

		return image;
	}
	
	RefPtr<RHI::StorageBuffer> TransientResourceSystem::AcquireBuffer(RGBufferRef resource)
	{
		VT_PROFILE_FUNCTION();

		{
			std::scoped_lock lock{ m_allocatedResourcesMutex };
			if (m_allocatedResources.contains(resource))
			{
				return m_allocatedResources.at(resource).resource.As<RHI::StorageBuffer>();
			}
		}

		// #TODO_Ivar: Switch to transient allocations
		auto allocator = RHI::GraphicsContext::GetDefaultAllocator(); //(bufferDesc.memoryUsage & RHI::MemoryUsage::CPUToGPU) != RHI::MemoryUsage::None ? RHI::GraphicsContext::GetDefaultAllocator() : RHI::GraphicsContext::GetTransientAllocator();

		const RGBufferDesc& desc = resource->GetDesc();
		RefPtr<RHI::StorageBuffer> buffer = RHI::StorageBuffer::Create(desc.count, desc.elementSize, desc.name, desc.usage, desc.memoryUsage, allocator);

		ResourceInfo info{};
		info.resource = buffer;
		info.isOriginal = true;

		{
			std::scoped_lock lock{ m_allocatedResourcesMutex };
			m_allocatedResources[resource] = info;
		}


		return buffer;
	}
	
	RefPtr<RHI::UniformBuffer> TransientResourceSystem::AcquireUniformBuffer(RGUniformBufferRef resource)
	{
		VT_PROFILE_FUNCTION();

		{
			std::scoped_lock lock{ m_allocatedResourcesMutex };
			if (m_allocatedResources.contains(resource))
			{
				return m_allocatedResources.at(resource).resource;
			}
		}

		const RGUniformBufferDesc& desc = resource->GetDesc();
		RefPtr<RHI::UniformBuffer> buffer = RHI::UniformBuffer::Create(static_cast<uint32_t>(desc.elementSize), nullptr, desc.count, desc.name);

		ResourceInfo info{};
		info.resource = buffer;
		info.isOriginal = true;

		{
			m_allocatedResources[resource] = info;
		}

		return buffer;
	}
	
	RefPtr<RHI::Image> TransientResourceSystem::GetTextureIfExists(RGTextureRef resource)
	{
		std::scoped_lock lock{ m_allocatedResourcesMutex };
		if (m_allocatedResources.contains(resource))
		{
			return m_allocatedResources.at(resource).resource.As<RHI::Image>();
		}

		return nullptr;
	}
	
	RefPtr<RHI::StorageBuffer> TransientResourceSystem::GetBufferIfExists(RGBufferRef resource)
	{
		std::scoped_lock lock{ m_allocatedResourcesMutex };
		if (m_allocatedResources.contains(resource))
		{
			return m_allocatedResources.at(resource).resource.As<RHI::StorageBuffer>();
		}

		return nullptr;
	}
	
	RefPtr<RHI::UniformBuffer> TransientResourceSystem::GetUniformBufferIfExists(RGUniformBufferRef resource)
	{
		std::scoped_lock lock{ m_allocatedResourcesMutex };
		if (m_allocatedResources.contains(resource))
		{
			return m_allocatedResources.at(resource).resource.As<RHI::UniformBuffer>();
		}

		return nullptr;
	}
	
	void TransientResourceSystem::SurrenderResource(RGResourceRef originalResource, size_t hash)
	{
		std::scoped_lock lock{ m_surrenderedResourcesMutex };
		m_surrenderedResources[hash].emplace_back(originalResource);
	}
	
	void TransientResourceSystem::AddExternalResource(RGResourceRef resource, RefPtr<RHI::RHIResource> rhiResource)
	{
		ResourceInfo info{};
		info.resource = rhiResource;
		info.isOriginal = true;

		std::scoped_lock lock{ m_allocatedResourcesMutex };
		m_allocatedResources[resource] = info;
	}
	
	const uint64_t TransientResourceSystem::GetTotalAllocatedSize() const
	{
		std::scoped_lock lock{ m_allocatedResourcesMutex };
		uint64_t result = 0;

		for (const auto& [handle, info] : m_allocatedResources)
		{
			if (info.isOriginal)
			{
				result += info.resource->GetByteSize();
			}
		}

		return result;
	}
}
