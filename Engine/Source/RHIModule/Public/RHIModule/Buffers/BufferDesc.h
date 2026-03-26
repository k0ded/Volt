#pragma once

#include <RHIModule/Core/RHICommon.h>

namespace Volt::RHI
{
	struct BufferDesc
	{
		uint64_t numElements;
		uint64_t elementSize;

		BufferUsage usage = BufferUsage::StorageBuffer;
		MemoryUsage memoryUsage = MemoryUsage::GPU;

		String debugName;
	};
}
