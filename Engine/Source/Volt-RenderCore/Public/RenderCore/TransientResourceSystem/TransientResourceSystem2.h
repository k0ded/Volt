#pragma once

#include "RenderCore/RenderGraph2/Resources/ResourceDeclarations.h"

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

	class TransientResourceSystem2
	{
	public:
		TransientResourceSystem2();
		~TransientResourceSystem2();

		TransientResourceSystem2(const TransientResourceSystem2& other) noexcept;
		TransientResourceSystem2(TransientResourceSystem2&& other) noexcept;
		TransientResourceSystem2& operator=(const TransientResourceSystem2& other) noexcept;
		TransientResourceSystem2& operator=(TransientResourceSystem2&& other) noexcept;

		RefPtr<RHI::Image> AcquireTexture(RGTextureRef resource);
		RefPtr<RHI::StorageBuffer> AcquireBuffer(RGBufferRef resource);
		RefPtr<RHI::UniformBuffer> AcquireUniformBuffer(RGUniformBufferRef resource);

		RefPtr<RHI::Image> GetTextureIfExists(RGTextureRef resource);
		RefPtr<RHI::StorageBuffer> GetBufferIfExists(RGBufferRef resource);
		RefPtr<RHI::UniformBuffer> GetUniformBufferIfExists(RGUniformBufferRef resource);

		void SurrenderResource(RGResourceRef originalResource, size_t hash);
		void AddExternalResource(RGResourceRef resource, RefPtr<RHI::RHIResource> rhiResource);

		const uint64_t GetTotalAllocatedSize() const;

	private:
		struct ResourceInfo
		{
			RefPtr<RHI::RHIResource> resource;
			bool isOriginal = false;
		};

		vt::map<RGResourceRef, ResourceInfo> m_allocatedResources;
		mutable std::mutex m_allocatedResourcesMutex;

		vt::map<size_t, Vector<RGResourceRef>> m_surrenderedResources;
		mutable std::mutex m_surrenderedResourcesMutex;
	};
}
