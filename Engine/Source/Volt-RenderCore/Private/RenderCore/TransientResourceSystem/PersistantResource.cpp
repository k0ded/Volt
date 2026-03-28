#include "rcpch.h"

#include "RenderCore/TransientResourceSystem/PersistantResource.h"

namespace Volt
{

	PersistantBufferResource::PersistantBufferResource(IntRef<RHI::Buffer> buffer)
		: m_viewCache(this),
		m_buffer(buffer)
	{

	}

	PersistantBufferResource::~PersistantBufferResource()
	{

	}

	PersistantTextureResource::PersistantTextureResource(IntRef<RHI::Image> image)
		: m_viewCache(this),
		m_image(image)
	{

	}

	PersistantUniformBufferResource::PersistantUniformBufferResource(IntRef<RHI::UniformBuffer> uniformBuffer)
		: m_viewCache(this),
		m_uniformBuffer(uniformBuffer)
	{

	}
}
