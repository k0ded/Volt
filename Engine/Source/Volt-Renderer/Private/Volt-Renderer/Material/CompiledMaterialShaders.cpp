#include "vrpch.h"

#include "Volt-Renderer/Material/CompiledMaterialShaders.h"

namespace Volt
{
	void CompiledMaterialShaders::Add(TypeTraits::TypeIndex shaderType, std::string&& compiledShader, const std::string& entryPoint)
	{
		CompiledMaterialShader& compiledMaterialShader = m_compiledShaders.emplace_back();
		compiledMaterialShader.shaderType = shaderType;
		compiledMaterialShader.compiledShader = std::move(compiledShader);
		compiledMaterialShader.entryPoint = entryPoint;
	}

	const CompiledMaterialShaders::CompiledMaterialShader& CompiledMaterialShaders::Get(TypeTraits::TypeIndex shaderType) const
	{
		auto it = m_compiledShaders.find_with_predicate([shaderType](const CompiledMaterialShader& compiledShader) 
		{
			return compiledShader.shaderType == shaderType;
		});

		VT_ENSURE(it != m_compiledShaders.end());
		return *it;
	}
}
