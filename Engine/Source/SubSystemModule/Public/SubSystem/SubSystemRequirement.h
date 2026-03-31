#pragma once

#include <CoreUtilities/CompileTimeString.h>
#include <CoreUtilities/String/StringView.h>
#include <CoreUtilities/Any.h>

template<typename T>
struct SubSystemRequirementTraits;

template<typename T>
struct SubSystemRequirementTraits
{
	static_assert(!std::is_same_v<T, T>, "SubSystemRequirementTraits not specialized for this type");
};

template<>
struct SubSystemRequirementTraits<bool>
{
	static bool Resolve(bool requiredValue, bool actualValue)
	{
		return requiredValue == actualValue;
	}
};

template<typename T, CompileTimeString Name>
struct SubSystemRequirementDefinition
{
	using Type = T;
	using Traits = SubSystemRequirementTraits<T>;
	inline static constexpr StringView RequirementName = Name;

	static bool Resolve(const Any& requiredValue, const Any& actualValue)
	{
		return Traits::Resolve(requiredValue.Cast<T>(), actualValue.Cast<T>());
	}
};

#define SUB_SYSTEM_REQUIREMENT_BOOL(requirementName) public SubSystemRequirementDefinition<bool, requirementName> {}
