#include "rhipch.h"

#include "RHIModule/Shader/Shader.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	IntRef<Shader> Shader::Create(const ShaderCreateInfo& createInfo)
	{
		return RHIModule::GetInstance().CreateShader(createInfo);
	}

	IntRef<Shader> Shader::CreateWithSource(const ShaderCreateInfo& createInfo, const String& source)
	{
		return RHIModule::GetInstance().CreateShaderWithSource(createInfo, source);
	}
}
