#include "rcpch.h"

#include "RenderCore/RenderGraph/Resources/RenderGraphUniformBuffer.h"

namespace Volt
{
	RGUniformBuffer::RGUniformBuffer(const RGUniformBufferDesc& desc)
		: m_desc(desc)
	{

	}

	RGResourceType RGUniformBuffer::GetResourceType() const
	{
		return RGResourceType::UniformBuffer;
	}

	RGUniformBufferSRV::RGUniformBufferSRV(RGUniformBufferRef uniformBuffer)
		: m_resource(uniformBuffer)
	{

	}
}
