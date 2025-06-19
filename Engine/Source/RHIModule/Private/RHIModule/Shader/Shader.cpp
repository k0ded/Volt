#include "rhipch.h"

#include "RHIModule/Shader/Shader.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<Shader> Shader::Create(const ShaderCreateInfo& createInfo)
	{
		return RHIModule::GetInstance().CreateShader(createInfo);
	}
}
