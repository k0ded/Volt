#pragma once

#include "RenderCore/Resources/BindlessResource.h"

namespace Volt
{
	class VTRC_API GrowingGPUBuffer
	{
	public:
		GrowingGPUBuffer(uint32_t initialCount, uint64_t elementSize, std::string_view name, RHI::BufferUsage bufferUsage = RHI::BufferUsage::StorageBuffer, RHI::MemoryUsage memoryUsage = RHI::MemoryUsage::GPU);
		~GrowingGPUBuffer();

		void GrowIfRequired(uint32_t requestedElementCount);
		void GrowIfRequired(uint64_t requestedElementCount);

		RefPtr<RHI::StorageBuffer> GetResource() const;

	private:
		BindlessResourceRef<RHI::StorageBuffer> m_buffer;
	};
}
