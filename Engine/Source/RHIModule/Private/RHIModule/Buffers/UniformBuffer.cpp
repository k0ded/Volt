#include "rhipch.h"

#include "RHIModule/Buffers/UniformBuffer.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<UniformBuffer> UniformBuffer::Create(const uint32_t size, const void* data, const uint32_t count, const std::string& name)
	{
		return RHIModule::GetInstance().CreateUniformBuffer(size, data, count, name);
	}
}
