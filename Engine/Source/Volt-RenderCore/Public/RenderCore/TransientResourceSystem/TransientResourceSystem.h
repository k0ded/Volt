#pragma once

#include "RenderCore/RenderGraph/Resources/ResourceDeclarations.h"

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

	class TransientResourceSystem
	{
	public:
		TransientResourceSystem();
		~TransientResourceSystem();

		TransientResourceSystem(const TransientResourceSystem& other) noexcept;
		TransientResourceSystem(TransientResourceSystem&& other) noexcept;
		TransientResourceSystem& operator=(const TransientResourceSystem& other) noexcept;
		TransientResourceSystem& operator=(TransientResourceSystem&& other) noexcept;

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
