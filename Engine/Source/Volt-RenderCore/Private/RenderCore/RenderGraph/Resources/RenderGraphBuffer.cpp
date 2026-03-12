#include "rcpch.h"

#include "RenderCore/RenderGraph/Resources/RenderGraphBuffer.h"
#include "RenderCore/TransientResourceSystem/PersistantResource.h"

#include <RHIModule/Graphics/GraphicsContext.h>

namespace Volt
{
	RGBuffer::RGBuffer(const RGBufferDesc& desc, uint32_t resourceId)
		: RGResource(resourceId),
		m_desc(desc), 
		m_rhiResource(nullptr)
	{
		m_isTransient = desc.memoryUsage == RHI::MemoryUsage::GPU;
		m_memoryRequirement = RHI::GraphicsContext::GetDevice()->GetBufferMemoryRequirement(m_desc);
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
