#include "rhipch.h"

#include "RHIModule/Buffers/Buffer.h"
#include "RHIModule/RHIModule.h"
#include "RHIModule/Memory/GPUAllocator.h"

namespace Volt::RHI
{
	RefPtr<Buffer> Buffer::Create(const BufferDesc& desc)
	{
		return RHIModule::GetInstance().CreateBuffer(desc);
	}
}
