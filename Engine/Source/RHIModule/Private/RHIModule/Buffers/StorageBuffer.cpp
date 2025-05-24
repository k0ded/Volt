#include "rhipch.h"

#include "RHIModule/Buffers/StorageBuffer.h"
#include "RHIModule/RHIModule.h"
#include "RHIModule/Memory/GPUAllocator.h"

namespace Volt::RHI
{
	RefPtr<StorageBuffer> StorageBuffer::Create(uint32_t count, uint64_t elementSize, const std::string& name, BufferUsage bufferUsage, MemoryUsage memoryUsage, RefPtr<GPUAllocator> allocator)
	{
		return RHIModule::GetInstance().CreateStorageBuffer(count, elementSize, name, bufferUsage, memoryUsage, allocator);
	}
}
