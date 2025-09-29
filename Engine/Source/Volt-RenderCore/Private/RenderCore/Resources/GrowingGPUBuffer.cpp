#include "rcpch.h"

#include "RenderCore/Resources/GrowingGPUBuffer.h"

#include <RHIModule/Buffers/StorageBuffer.h>

namespace Volt
{
	GrowingGPUBuffer::GrowingGPUBuffer(uint32_t initialCount, uint64_t elementSize, const std::string& name, RHI::BufferUsage bufferUsage, RHI::MemoryUsage memoryUsage)
	{
		RHI::BufferDesc desc{};
		desc.count = initialCount;
		desc.elementSize = elementSize;
		desc.debugName = name;
		desc.usage = bufferUsage;
		desc.memoryUsage = memoryUsage;

		m_buffer = RHI::StorageBuffer::Create(desc);
	}

	GrowingGPUBuffer::~GrowingGPUBuffer()
	{
		m_buffer = nullptr;
	}

	void GrowingGPUBuffer::GrowIfRequired(uint32_t requestedElementCount)
	{
		constexpr float GrowMultiplier = 1.5f;

		if (m_buffer->GetCount() < requestedElementCount)
		{
			m_buffer->ResizeWithCount(std::max(static_cast<uint32_t>(m_buffer->GetCount() * GrowMultiplier), requestedElementCount));
		}
	}

	void GrowingGPUBuffer::GrowIfRequired(uint64_t requestedElementCount)
	{
		GrowIfRequired(static_cast<uint32_t>(requestedElementCount));
	}

	RefPtr<RHI::StorageBuffer> GrowingGPUBuffer::GetResource() const
	{
		return m_buffer;
	}
}
