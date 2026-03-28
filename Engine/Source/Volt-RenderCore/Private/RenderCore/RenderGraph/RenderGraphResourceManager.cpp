#include "rcpch.h"

#include "RenderCore/RenderGraph/RenderGraphResourceManager.h"
#include "RenderCore/TransientResourceSystem/TransientResourceAllocator.h"

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	RenderGraphResourceManager::RenderGraphResourceManager(RenderGraphDataAllocator* dataAllocator)
	{
		m_persistantBufferResources.ReservePages(1);
		m_persistantTextureResources.ReservePages(1);
		m_persistantUniformBufferResources.ReservePages(1);

		m_transientBuffers.set_allocator({ dataAllocator });
		m_transientTextures.set_allocator({ dataAllocator });
		m_transientUniformBuffers.set_allocator({ dataAllocator });
	
		m_persistantBuffers.set_allocator({ dataAllocator });
		m_persistantTextures.set_allocator({ dataAllocator });
		m_persistantUniformBuffers.set_allocator({ dataAllocator });
	}

	RenderGraphResourceManager::~RenderGraphResourceManager()
	{
		VT_ASSERT(m_transientBuffers.empty());
		VT_ASSERT(m_transientTextures.empty());
		VT_ASSERT(m_transientUniformBuffers.empty());

		VT_ASSERT(m_persistantBuffers.empty());
		VT_ASSERT(m_persistantTextures.empty());
		VT_ASSERT(m_persistantUniformBuffers.empty());
	}

	RenderGraphResourceManager::RenderGraphResourceManager(RenderGraphResourceManager&& other) noexcept
	{
		m_persistantBufferResources = std::move(other.m_persistantBufferResources);
		m_persistantTextureResources = std::move(other.m_persistantTextureResources);

		m_transientBuffers = std::move(other.m_transientBuffers);
		m_transientTextures = std::move(other.m_transientTextures);
		m_transientUniformBuffers = std::move(other.m_transientUniformBuffers);

		m_persistantBuffers = std::move(other.m_persistantBuffers);
		m_persistantTextures = std::move(other.m_persistantTextures);
		m_persistantUniformBuffers = std::move(other.m_persistantUniformBuffers);
	}

	RenderGraphResourceManager& RenderGraphResourceManager::operator=(RenderGraphResourceManager&& other) noexcept
	{
		if (&other != this)
		{
			m_persistantBufferResources = std::move(other.m_persistantBufferResources);
			m_persistantTextureResources = std::move(other.m_persistantTextureResources);

			m_transientBuffers = std::move(other.m_transientBuffers);
			m_transientTextures = std::move(other.m_transientTextures);
			m_transientUniformBuffers = std::move(other.m_transientUniformBuffers);

			m_persistantBuffers = std::move(other.m_persistantBuffers);
			m_persistantTextures = std::move(other.m_persistantTextures);
			m_persistantUniformBuffers = std::move(other.m_persistantUniformBuffers);
		}

		return *this;
	}

	void RenderGraphResourceManager::Release()
	{
		for (const auto& resource : m_transientBuffers)
		{
			TransientResourceAllocator::Get().FreeBuffer(resource);
		}

		for (const auto& resource : m_transientTextures)
		{
			TransientResourceAllocator::Get().FreeTexture(resource);
		}

		for (const auto& resource : m_transientUniformBuffers)
		{
			TransientResourceAllocator::Get().FreeUniformBuffer(resource);
		}

		m_transientBuffers.clear();
		m_transientTextures.clear();
		m_transientUniformBuffers.clear();

		for (auto* resource : m_persistantBuffers)
		{
			m_persistantBufferResources.Free(resource);
		}

		for (auto* resource : m_persistantTextures)
		{
			m_persistantTextureResources.Free(resource);
		}

		for (auto* resource : m_persistantUniformBuffers)
		{
			m_persistantUniformBufferResources.Free(resource);
		}

		m_persistantBuffers.clear();
		m_persistantTextures.clear();
		m_persistantUniformBuffers.clear();
	}

	void RenderGraphResourceManager::AddExternalResource(RGResourceRef resource, IntRef<RHI::RHIResource> rhiResource)
	{
		VT_PROFILE_FUNCTION();

		if (resource->GetResourceType() == RGResourceType::Buffer)
		{
			PersistantBufferResource* persistantBuffer = m_persistantBufferResources.Allocate(rhiResource.As<RHI::Buffer>());
			
			RGBufferRef bufferResource = reinterpret_cast<RGBufferRef>(resource);
			bufferResource->AssignRHIResource(persistantBuffer);
		
			m_persistantBuffers.emplace_back(persistantBuffer);
		}
		else if (resource->GetResourceType() == RGResourceType::Texture)
		{
			PersistantTextureResource* persistantTexture = m_persistantTextureResources.Allocate(rhiResource.As<RHI::Image>());

			RGTextureRef textureResource = reinterpret_cast<RGTextureRef>(resource);
			textureResource->AssignRHIResource(persistantTexture);
		
			m_persistantTextures.emplace_back(persistantTexture);
		}
		else if (resource->GetResourceType() == RGResourceType::UniformBuffer)
		{
			PersistantUniformBufferResource* persistantUniformBuffer = m_persistantUniformBufferResources.Allocate(rhiResource.As<RHI::UniformBuffer>());

			RGUniformBufferRef uniformBufferResource = reinterpret_cast<RGUniformBufferRef>(resource);
			uniformBufferResource->AssignRHIResource(persistantUniformBuffer);
		
			m_persistantUniformBuffers.emplace_back(persistantUniformBuffer);
		}
		else
		{
			VT_ENSURE(false);
		}
	}

	void RenderGraphResourceManager::AllocateResource(RGTextureRef resource)
	{
		VT_PROFILE_FUNCTION();

		const RGTextureDesc& desc = resource->GetDesc();

		if (resource->IsTransient())
		{
			TransientTextureResourceRef texture = TransientResourceAllocator::Get().CreateTransientTexture(desc, resource->GetTransientAllocationRange());
			texture->GetRHITexture()->SetName(desc.debugName);
			resource->AssignRHIResource(texture);

			m_transientTextures.emplace_back(texture);
		}
		else
		{
			RGRHITextureResource* rhiResource = nullptr;

			if (!resource->m_isExtracted)
			{
				TransientTextureResourceRef texture = TransientResourceAllocator::Get().CreateTexture(desc);
				texture->GetRHITexture()->SetName(desc.debugName);
				m_transientTextures.emplace_back(texture);

				rhiResource = texture;
			}
			else
			{
				RHI::ImageDesc specification{};
				specification = desc;
				specification.initializeImage = false;

				IntRef<RHI::Image> image = RHI::Image::Create(specification);
				
				PersistantTextureResource* persistantTexture = m_persistantTextureResources.Allocate(image);
				rhiResource = persistantTexture;

				m_persistantTextures.emplace_back(persistantTexture);
			}

			resource->AssignRHIResource(rhiResource);
		}
	}
	
	void RenderGraphResourceManager::AllocateResource(RGBufferRef resource)
	{
		VT_PROFILE_FUNCTION();

		const RGBufferDesc& desc = resource->GetDesc();

		if (resource->IsTransient())
		{
			TransientBufferResourceRef buffer = TransientResourceAllocator::Get().CreateTransientBuffer(desc, resource->GetTransientAllocationRange());
			buffer->GetRHIBuffer()->SetName(desc.debugName);
			resource->AssignRHIResource(buffer);

			m_transientBuffers.emplace_back(buffer);
		}
		else
		{
			RGRHIBufferResource* rhiResource = nullptr;

			if (!resource->m_isExtracted)
			{
				TransientBufferResourceRef buffer = TransientResourceAllocator::Get().CreateBuffer(desc);
				buffer->GetRHIBuffer()->SetName(desc.debugName);
				m_transientBuffers.emplace_back(buffer);

				rhiResource = buffer;
			}
			else
			{
				IntRef<RHI::Buffer> buffer = RHI::Buffer::Create(desc);
				
				PersistantBufferResource* persistantBuffer = m_persistantBufferResources.Allocate(buffer);
				rhiResource = persistantBuffer;
			
				m_persistantBuffers.emplace_back(persistantBuffer);
			}

			resource->AssignRHIResource(rhiResource);
		}
	}
	
	void RenderGraphResourceManager::AllocateResource(RGUniformBufferRef resource)
	{
		VT_PROFILE_FUNCTION();

		const RGUniformBufferDesc& desc = resource->GetDesc();

		TransientUniformBufferResourceRef uniformBuffer = TransientResourceAllocator::Get().CreateUniformBuffer(desc);
		uniformBuffer->GetRHIUniformBuffer()->SetName(desc.debugName);

		m_transientUniformBuffers.emplace_back(uniformBuffer);
		resource->AssignRHIResource(uniformBuffer);
	}

	void RenderGraphResourceManager::ReserveTexturePages(uint32_t numPages)
	{
		TransientResourceAllocator::Get().ReserveTexturePages(numPages);
	}

	void RenderGraphResourceManager::ReserveBufferPages(uint32_t numPages)
	{
		TransientResourceAllocator::Get().ReserveBufferPages(numPages);
	}
}
