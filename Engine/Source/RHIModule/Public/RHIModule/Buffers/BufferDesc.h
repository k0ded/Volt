#pragma once

#include <RHIModule/Core/RHICommon.h>

namespace Volt::RHI
{
	struct BufferDesc
	{
		uint32_t count;
		uint64_t elementSize;

		BufferUsage usage = BufferUsage::StorageBuffer;
		MemoryUsage memoryUsage = MemoryUsage::GPU;

		std::string debugName;
	};
}
