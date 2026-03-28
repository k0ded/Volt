#pragma once

#include "RenderCore/RenderGraph/Resources/ResourceDeclarations.h"
#include "RenderCore/TransientResourceSystem/PersistantResource.h"

#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>

namespace Volt
{
	class TransientBufferResource;
	class TransientTextureResource;
	class TransientUniformBufferResource;

	class RenderGraphResourceManager
	{
	public:
		RenderGraphResourceManager(RenderGraphDataAllocator* dataAllocator);
		~RenderGraphResourceManager();

		RenderGraphResourceManager(RenderGraphResourceManager&& other) noexcept;
		RenderGraphResourceManager& operator=(RenderGraphResourceManager&& other) noexcept;

		void Release();
		void AddExternalResource(RGResourceRef resource, IntRef<RHI::RHIResource> rhiResource);

		void AllocateResource(RGTextureRef resource);
		void AllocateResource(RGBufferRef resource);
		void AllocateResource(RGUniformBufferRef resource);

		void ReserveTexturePages(uint32_t numPages);
		void ReserveBufferPages(uint32_t numPages);

	private:
		PagedAtomicArenaAllocator<PersistantBufferResource, 512> m_persistantBufferResources;
		PagedAtomicArenaAllocator<PersistantTextureResource, 512> m_persistantTextureResources;
		PagedAtomicArenaAllocator<PersistantUniformBufferResource, 512> m_persistantUniformBufferResources;

		RGVector<TransientBufferResource*> m_transientBuffers;
		RGVector<TransientTextureResource*> m_transientTextures;
		RGVector<TransientUniformBufferResource*> m_transientUniformBuffers;

		RGVector<PersistantBufferResource*> m_persistantBuffers;
		RGVector<PersistantTextureResource*> m_persistantTextures;
		RGVector<PersistantUniformBufferResource*> m_persistantUniformBuffers;
	};
}
