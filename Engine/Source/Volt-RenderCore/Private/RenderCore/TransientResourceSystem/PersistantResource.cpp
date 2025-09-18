#include "rcpch.h"

#include "RenderCore/TransientResourceSystem/PersistantResource.h"

namespace Volt
{

	PersistantBufferResource::PersistantBufferResource(RefPtr<RHI::StorageBuffer> buffer)
		: m_viewCache(this),
		m_buffer(buffer)
	{

	}

	PersistantTextureResource::PersistantTextureResource(RefPtr<RHI::Image> image)
		: m_viewCache(this),
		m_image(image)
	{

	}

	PersistantUniformBufferResource::PersistantUniformBufferResource(RefPtr<RHI::UniformBuffer> uniformBuffer)
		: m_viewCache(this),
		m_uniformBuffer(uniformBuffer)
	{

	}
}
