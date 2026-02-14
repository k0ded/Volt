#include "rcpch.h"

#include "RenderCore/RenderGraph/Resources/RenderGraphUniformBuffer.h"

namespace Volt
{
	RGUniformBuffer::RGUniformBuffer(const RGUniformBufferDesc& desc)
		: m_desc(desc), m_rhiResource(nullptr)
	{

	}

	RGResourceType RGUniformBuffer::GetResourceType() const
	{
		return RGResourceType::UniformBuffer;
	}

	RGUniformBufferSRV::RGUniformBufferSRV(const RGUniformBufferSRVDesc& desc)
		: m_desc(desc)
	{

	}
}
