#pragma once

#include "RenderCore/RenderGraph/Resources/ResourceDeclarations.h"
#include "RenderCore/TransientResourceSystem/PersistantResource.h"

#include <CoreUtilities/Allocators/ArenaAllocator.h>

namespace Volt
{
	class TransientBufferResource;
	class TransientTextureResource;
	class TransientUniformBufferResource;

	class RenderGraphResourceManager
	{
	public:
		RenderGraphResourceManager();
		~RenderGraphResourceManager();

		RenderGraphResourceManager(const RenderGraphResourceManager& other) noexcept;
		RenderGraphResourceManager(RenderGraphResourceManager&& other) noexcept;
		RenderGraphResourceManager& operator=(const RenderGraphResourceManager& other) noexcept;
		RenderGraphResourceManager& operator=(RenderGraphResourceManager&& other) noexcept;

		void AddExternalResource(RGResourceRef resource, RefPtr<RHI::RHIResource> rhiResource);
		void AddShaderParameterUniformBuffer(RGUniformBufferRef uniformBuffer);

		RGUniformBufferRef AquireShaderParameterUniformBuffer();

		void AllocateResource(RGTextureRef resource);
		void AllocateResource(RGBufferRef resource);
		void AllocateResource(RGUniformBufferRef resource);

	private:
		ArenaAllocator<PersistantBufferResource> m_persistantBufferResources;
		ArenaAllocator<PersistantTextureResource> m_persistantTextureResources;
		ArenaAllocator<PersistantUniformBufferResource> m_persistantUniformBufferResources;

		Vector<TransientBufferResource*> m_allocatedBuffers;
		Vector<TransientTextureResource*> m_allocatedTextures;
		Vector<TransientUniformBufferResource*> m_allocatedUniformBuffers;

		AtomicStack<RGUniformBufferRef> m_shaderParameterUniformBuffers;
	};
}
