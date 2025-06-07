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

	bool RGUniformBuffer::HasProducer(RGResourceUAV* uav) const
	{
		return m_isProduced;
	}

	bool RGUniformBuffer::HasProducer() const
	{
		return m_isProduced;
	}

	void RGUniformBuffer::AddProducer(Handle<RenderGraphPass> pass, RGResourceUAV* uav)
	{
		VT_ENSURE(!m_isProduced);
		producers.emplace_back(pass);
		m_isProduced = true;
	}

	void RGUniformBuffer::AddProducer(Handle<RenderGraphPass> pass)
	{
		VT_ENSURE(!m_isProduced);
		producers.emplace_back(pass);
		m_isProduced = true;
	}

	RGUniformBufferSRV::RGUniformBufferSRV(RGUniformBufferRef uniformBuffer)
		: m_resource(uniformBuffer)
	{

	}
}
