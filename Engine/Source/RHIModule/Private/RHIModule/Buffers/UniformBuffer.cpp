#include "rhipch.h"

#include "RHIModule/Buffers/UniformBuffer.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<UniformBuffer> UniformBuffer::Create(const UniformBufferDesc& uniformBufferDesc, const void* initialData)
	{
		return RHIModule::GetInstance().CreateUniformBuffer(uniformBufferDesc, initialData);
	}
}
