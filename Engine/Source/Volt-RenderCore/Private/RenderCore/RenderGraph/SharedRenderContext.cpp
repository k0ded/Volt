#include "rcpch.h"

#include "RenderCore/RenderGraph/SharedRenderContext.h"
#include "RenderCore/RenderGraph/RenderGraphCommon.h"
#include "RenderCore/Resources/BindlessResourcesManager.h"

#include <RHIModule/Buffers/UniformBuffer.h>
#include <RHIModule/Buffers/StorageBuffer.h>

namespace Volt
{
	SharedRenderContext::~SharedRenderContext()
	{
		VT_ENSURE(!m_isRenderGraphConstantsMapped);
	}

	SharedRenderContext::SharedRenderContext(SharedRenderContext&& other) noexcept
		: m_isRenderGraphConstantsMapped(other.m_isRenderGraphConstantsMapped),
		m_mappedRenderGraphConstantsPointer(other.m_mappedRenderGraphConstantsPointer),
		m_renderGraphConstantsBuffer(std::move(other.m_renderGraphConstantsBuffer))
	{
	}

	SharedRenderContext& SharedRenderContext::operator=(SharedRenderContext&& other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}

		m_isRenderGraphConstantsMapped = other.m_isRenderGraphConstantsMapped;
		m_mappedRenderGraphConstantsPointer = other.m_mappedRenderGraphConstantsPointer;
		m_renderGraphConstantsBuffer = std::move(other.m_renderGraphConstantsBuffer);
	
		return *this;
	}

	void SharedRenderContext::BeginContext()
	{
	}

	void SharedRenderContext::EndContext()
	{
		if (m_isRenderGraphConstantsMapped)
		{
			m_renderGraphConstantsBuffer->Unmap();
			m_mappedRenderGraphConstantsPointer = nullptr;
			m_isRenderGraphConstantsMapped = false;
		}
	}

	uint8_t* SharedRenderContext::GetRenderGraphConstantsPointer(uint32_t passIndex)
	{
		if (!m_isRenderGraphConstantsMapped)
		{
			m_mappedRenderGraphConstantsPointer = m_renderGraphConstantsBuffer->Map<uint8_t>();
			m_isRenderGraphConstantsMapped = true;
		}

		const uint32_t offset = RenderGraphCommon::MAX_PASS_CONSTANTS_SIZE * passIndex;
		return &m_mappedRenderGraphConstantsPointer[offset];
	}

	void SharedRenderContext::SetRenderGraphConstantsBuffer(RawPtr<RHI::UniformBuffer> constantsBuffer)
	{
		m_renderGraphConstantsBuffer = constantsBuffer;
	}
}
