#include "rcpch.h"

#include "RenderCore/TransientResourceSystem/TransientResource.h"

namespace Volt
{
	TransientBufferResource::TransientBufferResource(RefPtr<RHI::StorageBuffer> buffer, size_t hash, uint64_t framesToKeepAlive)
		: m_buffer(buffer),
		m_hash(hash),
		m_viewCache(this),
		m_acquired(false),
		m_framesToKeepAlive(framesToKeepAlive),
		m_frameReleasedIndex(0)
	{

	}

	TransientTextureResource::TransientTextureResource(RefPtr<RHI::Image> image, size_t hash, uint64_t framesToKeepAlive)
		: m_image(image),
		m_hash(hash),
		m_viewCache(this),
		m_acquired(false),
		m_framesToKeepAlive(framesToKeepAlive),
		m_frameReleasedIndex(0)
	{

	}

	TransientUniformBufferResource::TransientUniformBufferResource(RefPtr<RHI::UniformBuffer> uniformBuffer, size_t hash, uint64_t framesToKeepAlive)
		: m_uniformBuffer(uniformBuffer),
		m_hash(hash),
		m_viewCache(this),
		m_acquired(false),
		m_framesToKeepAlive(framesToKeepAlive),
		m_frameReleasedIndex(0)
	{

	}

}
