#include "rcpch.h"

#include "RenderCore/TransientResourceSystem/TransientResource.h"

namespace Volt
{
	TransientUniformBufferResource::TransientUniformBufferResource(IntRef<RHI::UniformBuffer> uniformBuffer, size_t hash, uint64_t framesToKeepAlive)
		: m_uniformBuffer(uniformBuffer),
		m_hash(hash),
		m_viewCache(this),
		m_acquired(false),
		m_framesToKeepAlive(framesToKeepAlive),
		m_frameReleasedIndex(0)
	{

	}

	TransientBufferResource::TransientBufferResource(IntRef<RHI::Buffer> buffer, size_t hash, uint64_t framesToKeepAlive, bool isTransientlyAllocated)
		: m_buffer(buffer),
		m_hash(hash),
		m_viewCache(this),
		m_acquired(false),
		m_framesToKeepAlive(framesToKeepAlive),
		m_frameReleasedIndex(0),
		m_isTransientlyAllocated(isTransientlyAllocated)
	{

	}

	TransientTextureResource::TransientTextureResource(IntRef<RHI::Image> image, size_t hash, uint64_t framesToKeepAlive, bool isTransientlyAllocated)
		: m_image(image),
		m_hash(hash),
		m_viewCache(this),
		m_acquired(false),
		m_framesToKeepAlive(framesToKeepAlive),
		m_frameReleasedIndex(0),
		m_isTransientlyAllocated(isTransientlyAllocated)
	{

	}
}
