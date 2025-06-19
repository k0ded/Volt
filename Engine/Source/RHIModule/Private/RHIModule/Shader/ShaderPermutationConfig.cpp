#include "rhipch.h"

#include "RHIModule/Shader/ShaderPermutationConfig.h"

#include <CoreUtilities/StringUtility.h>

namespace Volt::RHI
{
	void ShaderPermutationConfig::AddPermutation(const std::string& name, const std::string& value)
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

	Vector<std::wstring> ShaderPermutationConfig::GetPermutationsWideStr() const
	{
		Vector<std::wstring> result;
		for (const auto& permutation : m_permutationDefines)
		{
			result.emplace_back(Utility::ToWString(permutation));
		}

		return result;
	}
}
