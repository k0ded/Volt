#include "rhipch.h"

#include "RHIModule/Buffers/VertexBuffer.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<VertexBuffer> VertexBuffer::Create(const void* data, const uint32_t size, const uint32_t stride)
	{
		return RHIModule::GetInstance().CreateVertexBuffer(data, size, stride);
	}
}
