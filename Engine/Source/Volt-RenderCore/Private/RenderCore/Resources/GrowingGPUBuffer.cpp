#include "rcpch.h"

#include "RenderCore/Resources/GrowingGPUBuffer.h"

#include <RHIModule/Buffers/StorageBuffer.h>

namespace Volt
{
	GrowingGPUBuffer::GrowingGPUBuffer(uint32_t initialCount, uint64_t elementSize, std::string_view name, RHI::BufferUsage bufferUsage, RHI::MemoryUsage memoryUsage)
	{
		m_buffer = BindlessResource<RHI::StorageBuffer>::CreateRef(initialCount, elementSize, name, bufferUsage, memoryUsage);
	}

	GrowingGPUBuffer::~GrowingGPUBuffer()
	{
		m_buffer = nullptr;
	}

	void GrowingGPUBuffer::GrowIfRequired(uint32_t requestedElementCount)
	{
		constexpr float GrowMultiplier = 1.5f;

		if (m_buffer->GetResource()->GetCount() < requestedElementCount)
		{
			m_buffer->GetResource()->ResizeWithCount(std::max(static_cast<uint32_t>(m_buffer->GetResource()->GetCount() * GrowMultiplier), requestedElementCount));
			m_buffer->MarkAsDirty();
		}
	}

	void GrowingGPUBuffer::GrowIfRequired(uint64_t requestedElementCount)
	{
		GrowIfRequired(static_cast<uint32_t>(requestedElementCount));
	}

	RefPtr<RHI::StorageBuffer> GrowingGPUBuffer::GetResource() const
	{
		return m_buffer->GetResource();
	}
}
