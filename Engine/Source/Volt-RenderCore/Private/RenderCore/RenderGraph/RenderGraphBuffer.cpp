#include "rcpch.h"

#include "RenderCore/RenderGraph/Resources/RenderGraphBuffer.h"

namespace Volt
{
	RGBuffer::RGBuffer(const RGBufferDesc& desc)
		: m_desc(desc)
	{

	}

	RGResourceType RGBuffer::GetResourceType() const
	{
		return RGResourceType::Buffer;
	}

	bool RGBuffer::HasProducer(RGResourceUAV* uav) const
	{
		return m_isProduced;
	}

	bool RGBuffer::HasProducer() const
	{
		return m_isProduced;
	}

	void RGBuffer::AddProducer(Handle<RenderGraphPass> pass, RGResourceUAV* uav)
	{
		VT_ENSURE(!m_isProduced);
		producers.emplace_back(pass);
		m_isProduced = true;
	}

	void RGBuffer::AddProducer(Handle<RenderGraphPass> pass)
	{
		VT_ENSURE(!m_isProduced);
		producers.emplace_back(pass);
		m_isProduced = true;
	}

	RGBufferSRV::RGBufferSRV(const RGBufferSRVDesc& desc)
		: m_desc(desc)
	{

	}

	RGBufferUAV::RGBufferUAV(const RGBufferUAVDesc& desc)
		: m_desc(desc)
	{

	}
}
