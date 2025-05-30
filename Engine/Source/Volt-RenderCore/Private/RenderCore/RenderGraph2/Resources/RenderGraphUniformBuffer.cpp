#include "rcpch.h"

#include "RenderCore/RenderGraph2/Resources/RenderGraphUniformBuffer.h"

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
}
