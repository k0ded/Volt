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

		m_allocatedBuffers.set_allocator({ dataAllocator });
		m_allocatedTextures.set_allocator({ dataAllocator });
		m_allocatedUniformBuffers.set_allocator({ dataAllocator });
	}

	RenderGraphResourceManager::~RenderGraphResourceManager()
	{
		VT_ASSERT(m_allocatedBuffers.empty());
		VT_ASSERT(m_allocatedTextures.empty());
		VT_ASSERT(m_allocatedUniformBuffers.empty());
	}

	RenderGraphResourceManager::RenderGraphResourceManager(RenderGraphResourceManager && other) noexcept
	{
		m_persistantBufferResources = std::move(other.m_persistantBufferResources);
		m_persistantTextureResources = std::move(other.m_persistantTextureResources);

		m_allocatedBuffers = std::move(other.m_allocatedBuffers);
		m_allocatedTextures = std::move(other.m_allocatedTextures);
		m_allocatedUniformBuffers = std::move(other.m_allocatedUniformBuffers);
	}

	RenderGraphResourceManager& RenderGraphResourceManager::operator=(RenderGraphResourceManager&& other) noexcept
	{
		m_persistantBufferResources = std::move(other.m_persistantBufferResources);
		m_persistantTextureResources = std::move(other.m_persistantTextureResources);

		m_allocatedBuffers = std::move(other.m_allocatedBuffers);
		m_allocatedTextures = std::move(other.m_allocatedTextures);
		m_allocatedUniformBuffers = std::move(other.m_allocatedUniformBuffers);
		return *this;
	}

	void RenderGraphResourceManager::Release()
	{
		for (const auto& resource : m_allocatedBuffers)
		{
			TransientResourceAllocator::Get().FreeBuffer(resource);
		}

		for (const auto& resource : m_allocatedTextures)
		{
			TransientResourceAllocator::Get().FreeTexture(resource);
		}

		for (const auto& resource : m_allocatedUniformBuffers)
		{
			TransientResourceAllocator::Get().FreeUniformBuffer(resource);
		}

		m_allocatedBuffers.clear();
		m_allocatedTextures.clear();
		m_allocatedUniformBuffers.clear();
	}

	void RenderGraphResourceManager::AddExternalResource(RGResourceRef resource, RefPtr<RHI::RHIResource> rhiResource)
	{
		VT_PROFILE_FUNCTION();

		if (resource->GetResourceType() == RGResourceType::Buffer)
		{
			PersistantBufferResource* persistantBuffer = m_persistantBufferResources.Allocate(rhiResource.As<RHI::Buffer>());
			
			RGBufferRef bufferResource = reinterpret_cast<RGBufferRef>(resource);
			bufferResource->AssignRHIResource(persistantBuffer);
		}
		else if (resource->GetResourceType() == RGResourceType::Texture)
		{
			PersistantTextureResource* persistantTexture = m_persistantTextureResources.Allocate(rhiResource.As<RHI::Image>());

			RGTextureRef textureResource = reinterpret_cast<RGTextureRef>(resource);
			textureResource->AssignRHIResource(persistantTexture);
		}
		else if (resource->GetResourceType() == RGResourceType::UniformBuffer)
		{
			PersistantUniformBufferResource* persistantUniformBuffer = m_persistantUniformBufferResources.Allocate(rhiResource.As<RHI::UniformBuffer>());

			RGUniformBufferRef uniformBufferResource = reinterpret_cast<RGUniformBufferRef>(resource);
			uniformBufferResource->AssignRHIResource(persistantUniformBuffer);
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

		RGRHITextureResource* rhiResource = nullptr;

		if (!resource->m_isExtracted)
		{
			TransientTextureResourceRef texture = TransientResourceAllocator::Get().CreateTexture(desc);
			texture->GetRHITexture()->SetName(desc.debugName);
			m_allocatedTextures.emplace_back(texture);

			rhiResource = texture;
		}
		else
		{
			RHI::ImageDesc specification{};
			specification = desc;
			specification.initializeImage = false;

			RefPtr<RHI::Image> image = RHI::Image::Create(specification);
			rhiResource = m_persistantTextureResources.Allocate(image);
		}

		resource->AssignRHIResource(rhiResource);
	}
	
	void RenderGraphResourceManager::AllocateResource(RGBufferRef resource)
	{
		VT_PROFILE_FUNCTION();

		const RGBufferDesc& desc = resource->GetDesc();

		RGRHIBufferResource* rhiResource = nullptr;

		if (!resource->m_isExtracted)
		{
			TransientBufferResourceRef buffer = TransientResourceAllocator::Get().CreateBuffer(desc);
			buffer->GetRHIBuffer()->SetName(desc.debugName);
			m_allocatedBuffers.emplace_back(buffer);

			rhiResource = buffer;
		}
		else
		{
			RefPtr<RHI::Buffer> buffer = RHI::Buffer::Create(desc);
			rhiResource = m_persistantBufferResources.Allocate(buffer);
		}

		resource->AssignRHIResource(rhiResource);
	}
	
	void RenderGraphResourceManager::AllocateResource(RGUniformBufferRef resource)
	{
		VT_PROFILE_FUNCTION();

		const RGUniformBufferDesc& desc = resource->GetDesc();

		TransientUniformBufferResourceRef uniformBuffer = TransientResourceAllocator::Get().CreateUniformBuffer(desc);
		uniformBuffer->GetRHIUniformBuffer()->SetName(desc.debugName);

		m_allocatedUniformBuffers.emplace_back(uniformBuffer);
		resource->AssignRHIResource(uniformBuffer);
	}
}
