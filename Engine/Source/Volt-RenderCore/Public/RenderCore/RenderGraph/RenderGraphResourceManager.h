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
		void AddExternalResource(RGResourceRef resource, RefPtr<RHI::RHIResource> rhiResource);

		void AllocateResource(RGTextureRef resource);
		void AllocateResource(RGBufferRef resource);
		void AllocateResource(RGUniformBufferRef resource);

	private:
		PagedAtomicArenaAllocator<PersistantBufferResource, 512> m_persistantBufferResources;
		PagedAtomicArenaAllocator<PersistantTextureResource, 512> m_persistantTextureResources;
		PagedAtomicArenaAllocator<PersistantUniformBufferResource, 512> m_persistantUniformBufferResources;

		RGVector<TransientBufferResource*> m_allocatedBuffers;
		RGVector<TransientTextureResource*> m_allocatedTextures;
		RGVector<TransientUniformBufferResource*> m_allocatedUniformBuffers;
	};
}
