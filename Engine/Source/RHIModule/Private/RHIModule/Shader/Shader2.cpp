#include "rhipch.h"

#include "RHIModule/Shader/Shader2.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<Shader2> Shader2::Create(const ShaderCreateInfo& createInfo)
	{
		return RHIModule::GetInstance().CreateShader2(createInfo);
	}
}
