#pragma once

#include "SubSystem/SubSystemRequirement.h"

#include <CoreUtilities/Containers/Vector.h>

class SubSystemRequirementList
{
public:
	template<typename T>
	void Requires(typename T::Type requiredValue);
	void RequiresIsTrue(bool value);

private:
	using ResolveRequirementFunc = bool(*)(const Any& requiredValue, const Any& actualValue);

	struct Requirement
	{
		TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<void>();
		Any requiredValue;
		ResolveRequirementFunc resolveFunc;
	};

	Vector<Requirement> m_requirements;
};

template<typename T>
void SubSystemRequirementList::Requires(typename T::Type requiredValue)
{
	constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();

	Requirement& newRequirement = m_requirements.emplace_back();
	newRequirement.requiredValue = requiredValue;
	newRequirement.resolveFunc = T::Resolve;
	newRequirement.typeIndex = typeIndex;
}
