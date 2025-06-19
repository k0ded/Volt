#include "rhipch.h"

#include "RHIModule/Buffers/IndexBuffer.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<IndexBuffer> IndexBuffer::Create(std::span<uint32_t> indices)
	{
		return RHIModule::GetInstance().CreateIndexBuffer(indices);
	}
}
