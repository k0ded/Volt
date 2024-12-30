#pragma once

#include "RenderCore/RenderGraph/Resources/RenderGraphResourceHandle.h"

#include <CoreUtilities/Pointers/RawPtr.h>
#include <CoreUtilities/Containers/ThreadSafeMap.h>

namespace Volt
{
	namespace RHI
	{
		class Image;
		class StorageBuffer;
		class UniformBuffer;
		class RHIResource;
	}

	struct RenderGraphImageDesc;
	struct RenderGraphBufferDesc;

	class TransientResourceSystem
	{
	public:
		TransientResourceSystem();
		~TransientResourceSystem();

		TransientResourceSystem(const TransientResourceSystem& other) noexcept;
		TransientResourceSystem(TransientResourceSystem&& other) noexcept;
		TransientResourceSystem& operator=(const TransientResourceSystem& other) noexcept;
		TransientResourceSystem& operator=(TransientResourceSystem&& other) noexcept;

		RawPtr<RHI::Image> AcquireImage(RenderGraphImageHandle resourceHandle, const RenderGraphImageDesc& imageDesc);
		RawPtr<RHI::StorageBuffer> AcquireBuffer(RenderGraphBufferHandle resourceHandle, const RenderGraphBufferDesc& bufferDesc);
		RawPtr<RHI::UniformBuffer> AcquireUniformBuffer(RenderGraphUniformBufferHandle resourceHandle, const RenderGraphBufferDesc& bufferDesc);

		RefPtr<RHI::Image> AcquireImageRef(RenderGraphImageHandle resourceHandle, const RenderGraphImageDesc& imageDesc);
		RefPtr<RHI::StorageBuffer> AcquireBufferRef(RenderGraphBufferHandle resourceHandle, const RenderGraphBufferDesc& bufferDesc);
		RefPtr<RHI::UniformBuffer> AcquireUniformBufferRef(RenderGraphUniformBufferHandle resourceHandle, const RenderGraphBufferDesc& bufferDesc);

		RefPtr<RHI::Image> GetImageIfExists(RenderGraphImageHandle resourceHandle, const RenderGraphImageDesc& imageDesc);
		RefPtr<RHI::StorageBuffer> GetBufferIfExists(RenderGraphBufferHandle resourceHandle, const RenderGraphBufferDesc& imageDesc);
		RefPtr<RHI::UniformBuffer> GetUniformBufferIfExists(RenderGraphUniformBufferHandle resourceHandle, const RenderGraphBufferDesc& imageDesc);

		void SurrenderResource(RenderGraphResourceHandle originalResource, size_t hash);
		void AddExternalResource(RenderGraphResourceHandle resourceHandle, RefPtr<RHI::RHIResource> resource);

		const uint64_t GetTotalAllocatedSize() const;

	private:
		struct ResourceInfo
		{
			RefPtr<RHI::RHIResource> resource;
			bool isOriginal = false;
		};

		vt::map<RenderGraphResourceHandle, ResourceInfo> m_allocatedResources;
		mutable std::mutex m_allocatedResourcesMutex;

		vt::map<size_t, Vector<RenderGraphResourceHandle>> m_surrenderedResources;
		mutable std::mutex m_surrenderedResourcesMutex;
	};
}
