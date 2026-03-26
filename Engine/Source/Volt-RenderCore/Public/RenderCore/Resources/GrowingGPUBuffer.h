#pragma once

#include "RenderCore/Config.h"

#include <RHIModule/Buffers/Buffer.h>

namespace Volt
{
	class VTRC_API GrowingGPUBuffer
	{
	public:
		GrowingGPUBuffer(uint32_t initialCount, uint64_t elementSize, const String& name, RHI::BufferUsage bufferUsage = RHI::BufferUsage::StorageBuffer, RHI::MemoryUsage memoryUsage = RHI::MemoryUsage::GPU);
		~GrowingGPUBuffer();

		void GrowIfRequired(uint32_t requestedElementCount);
		void GrowIfRequired(uint64_t requestedElementCount);

		uint64_t GetByteSize();

		IntRef<RHI::Buffer> GetResource() const;

	private:
		IntRef<RHI::Buffer> m_buffer;
	};
}
