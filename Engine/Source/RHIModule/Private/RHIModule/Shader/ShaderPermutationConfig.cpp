#include "rhipch.h"

#include "RHIModule/Shader/ShaderPermutationConfig.h"

namespace Volt::RHI
{
	void ShaderPermutationConfig::AddPermutation(const String& name, const String& value)
	{
		// Each permutation is a macro that is fed into the shader compiler.
		// The format for these should be <name>=<value>, and if no value is defined it should just be <name>
	
		auto& permutationString = m_permutationDefines.emplace_back();
		permutationString = name;
		
		if (!value.empty())
		{
			permutationString += "=" + value;
		}
	}

	Vector<WString> ShaderPermutationConfig::GetPermutationsWideStr() const
	{
		Vector<WString> result;
		for (const auto& permutation : m_permutationDefines)
		{
			result.emplace_back(WString::CtorConvert(), permutation);
		}

		return result;
	}
}
