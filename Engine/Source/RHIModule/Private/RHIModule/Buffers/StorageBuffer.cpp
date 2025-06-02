#include "rhipch.h"

#include "RHIModule/Buffers/StorageBuffer.h"
#include "RHIModule/RHIModule.h"
#include "RHIModule/Memory/GPUAllocator.h"

namespace Volt::RHI
{
	RefPtr<StorageBuffer> StorageBuffer::Create(const BufferDesc& desc, RefPtr<GPUAllocator> allocator)
	{
		return RHIModule::GetInstance().CreateStorageBuffer(desc, allocator);
	}
}
