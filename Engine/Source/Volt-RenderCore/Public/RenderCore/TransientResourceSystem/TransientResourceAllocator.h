#pragma once

#include "RenderCore/Config.h"
#include "RenderCore/RenderGraph/Resources/ResourceDeclarations.h"
#include "RenderCore/TransientResourceSystem/TransientResource.h"

#include <RHIModule/Buffers/Buffer.h>
#include <RHIModule/Images/Image.h>
#include <RHIModule/Memory/TransientHeap.h>

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

		TransientBufferResourceRef CreateTransientBuffer(const RGBufferDesc& desc, const PagedAllocatedRange& requiredRange);
		TransientTextureResourceRef CreateTransientTexture(const RGTextureDesc& desc, const PagedAllocatedRange& requiredRange);

		TransientBufferResourceRef CreateBuffer(const RGBufferDesc& desc);
		void FreeBuffer(TransientBufferResourceRef transientBuffer);

		TransientTextureResourceRef CreateTexture(const RGTextureDesc& desc);
		void FreeTexture(TransientTextureResourceRef transientTexture);

		TransientUniformBufferResourceRef CreateUniformBuffer(const RGUniformBufferDesc& desc);
		void FreeUniformBuffer(TransientUniformBufferResourceRef transientUniformBuffer);

		void OnPreRender(uint64_t frameIndex);

		void ReserveTexturePages(uint32_t numPages);
		void ReserveBufferPages(uint32_t numPages);

		uint64_t GetPageSize() const;

		VT_INLINE static TransientResourceAllocator& Get() { return *s_instance; }

	private:
		inline static TransientResourceAllocator* s_instance = nullptr;

		void CreateHeaps();

		uint64_t m_frameIndex = 0;

		Vector<TransientBufferResourceRef> m_bufferCache;
		Vector<TransientTextureResourceRef> m_textureCache;
		Vector<TransientUniformBufferResourceRef> m_uniformBufferCache;

		IntRef<RHI::TransientHeap> m_bufferHeap;
		IntRef<RHI::TransientHeap> m_textureHeap;

		PagedAtomicArenaAllocator<TransientBufferResource, 512> m_transientBufferAllocator;
		PagedAtomicArenaAllocator<TransientTextureResource, 512> m_transientTextureAllocator;
		PagedAtomicArenaAllocator<TransientUniformBufferResource, 512> m_transientUniformBufferAllocator;
	};
}
