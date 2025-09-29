#pragma once

#include "RenderCore/Config.h"

#include <RHIModule/Buffers/StorageBuffer.h>

namespace Volt
{
	class VTRC_API GrowingGPUBuffer
	{
	public:
		GrowingGPUBuffer(uint32_t initialCount, uint64_t elementSize, const std::string& name, RHI::BufferUsage bufferUsage = RHI::BufferUsage::StorageBuffer, RHI::MemoryUsage memoryUsage = RHI::MemoryUsage::GPU);
		~GrowingGPUBuffer();

		void GrowIfRequired(uint32_t requestedElementCount);
		void GrowIfRequired(uint64_t requestedElementCount);

		RefPtr<RHI::StorageBuffer> GetResource() const;

	private:
		RefPtr<RHI::StorageBuffer> m_buffer;
	};
}
