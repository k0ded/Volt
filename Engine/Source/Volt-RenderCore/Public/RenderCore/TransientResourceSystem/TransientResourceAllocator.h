#pragma once

#include "RenderCore/Config.h"
#include "RenderCore/RenderGraph/Resources/ResourceDeclarations.h"
#include "RenderCore/TransientResourceSystem/TransientResource.h"

#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Images/Image.h>

#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>

namespace Volt
{
	using TransientBufferResourceRef = TransientBufferResource*;
	using TransientTextureResourceRef = TransientTextureResource*;
	using TransientUniformBufferResourceRef = TransientUniformBufferResource*;

	class VTRC_API TransientResourceAllocator
	{
	public:
		TransientResourceAllocator();
		~TransientResourceAllocator();

		TransientBufferResourceRef CreateBuffer(const RGBufferDesc& desc);
		void FreeBuffer(TransientBufferResourceRef transientBuffer);

		TransientTextureResourceRef CreateTexture(const RGTextureDesc& desc);
		void FreeTexture(TransientTextureResourceRef transientTexture);

		TransientUniformBufferResourceRef CreateUniformBuffer(const RGUniformBufferDesc& desc);
		void FreeUniformBuffer(TransientUniformBufferResourceRef transientUniformBuffer);

		void OnPreRender(uint64_t frameIndex);

		VT_INLINE static TransientResourceAllocator& Get() { return *s_instance; }

	private:
		inline static TransientResourceAllocator* s_instance = nullptr;

		uint64_t m_frameIndex = 0;

		Vector<TransientBufferResourceRef> m_bufferCache;
		Vector<TransientTextureResourceRef> m_textureCache;
		Vector<TransientUniformBufferResourceRef> m_uniformBufferCache;

		PagedAtomicArenaAllocator<TransientBufferResource, 512> m_transientBufferAllocator;
		PagedAtomicArenaAllocator<TransientTextureResource, 512> m_transientTextureAllocator;
		PagedAtomicArenaAllocator<TransientUniformBufferResource, 512> m_transientUniformBufferAllocator;
	};
}
