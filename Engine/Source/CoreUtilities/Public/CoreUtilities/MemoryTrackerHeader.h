#pragma once

#include <cstdint>

struct MemoryTrackerHeader
{
	void* basePtr;
	uint64_t alignment;
	uint64_t size;
	uint32_t memoryTagIndex;
};
