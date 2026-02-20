#include "rhipch.h"

#include "RHIModule/Buffers/TransientBuffer.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<TransientBuffer> TransientBuffer::Create(const BufferDesc& desc)
	{
		return RHIModule::GetInstance().CreateTransientBuffer(desc);
	}
}
