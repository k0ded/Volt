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

	RGBufferSRV::RGBufferSRV(const RGBufferSRVDesc& desc)
		: m_desc(desc)
	{

	}

	RGBufferUAV::RGBufferUAV(const RGBufferUAVDesc& desc)
		: m_desc(desc)
	{

	}
}
