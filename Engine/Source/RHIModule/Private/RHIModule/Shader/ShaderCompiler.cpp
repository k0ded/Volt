#include "rhipch.h"
#include "RHIModule/Shader/ShaderCompiler.h"

#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	ShaderCompiler::ShaderCompiler()
	{
		s_instance = this;
	}

	ShaderCompiler::~ShaderCompiler()
	{
		s_instance = nullptr;
	}

	void ShaderCompiler::AddMacro(const String& macroName)
	{
		s_instance->AddMacroImpl(macroName);
	}

	void ShaderCompiler::RemoveMacro(StringView macroName)
	{
		s_instance->RemoveMacroImpl(macroName);
	}

	IntRef<ShaderCompiler> ShaderCompiler::Create(const ShaderCompilerCreateInfo& createInfo)
	{
		return RHIModule::GetInstance().CreateShaderCompiler(createInfo);
	}

	ShaderCompiler::CompilationResultData ShaderCompiler::TryCompile(const Specification& specification)
	{
		return s_instance->TryCompileImpl(specification);
	}
}
