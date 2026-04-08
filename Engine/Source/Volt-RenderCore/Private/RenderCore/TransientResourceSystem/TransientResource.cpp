#include "rcpch.h"

#include "RenderCore/TransientResourceSystem/TransientResource.h"

namespace Volt
{
	TransientUniformBufferResource::TransientUniformBufferResource(IntRef<RHI::UniformBuffer> uniformBuffer, size_t hash, uint64_t framesToKeepAlive)
		: m_viewCache(this),
		m_uniformBuffer(uniformBuffer),
		m_hash(hash),
		m_frameReleasedIndex(0),
		m_framesToKeepAlive(framesToKeepAlive),
		m_acquired(false)
	{

	}

	TransientBufferResource::TransientBufferResource(IntRef<RHI::Buffer> buffer, size_t hash, uint64_t framesToKeepAlive, bool isTransientlyAllocated)
		: m_viewCache(this),
		m_buffer(buffer),
		m_hash(hash),
		m_frameReleasedIndex(0),
		m_framesToKeepAlive(framesToKeepAlive),
		m_isTransientlyAllocated(isTransientlyAllocated),
		m_acquired(false)
	{

	}

	TransientTextureResource::TransientTextureResource(IntRef<RHI::Image> image, size_t hash, uint64_t framesToKeepAlive, bool isTransientlyAllocated)
		: m_viewCache(this),
		m_image(image),
		m_hash(hash),
		m_frameReleasedIndex(0),
		m_framesToKeepAlive(framesToKeepAlive),
		m_isTransientlyAllocated(isTransientlyAllocated),
		m_acquired(false)
	{

	}
}
