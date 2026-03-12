#include "rhipch.h"

#include "RHIModule/Shader/Shader.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<Shader> Shader::Create(const ShaderCreateInfo& createInfo)
	{
		return RHIModule::GetInstance().CreateShader(createInfo);
	}

	RefPtr<Shader> Shader::CreateWithSource(const ShaderCreateInfo& createInfo, const std::string& source)
	{
		return RHIModule::GetInstance().CreateShaderWithSource(createInfo, source);
	}
}
