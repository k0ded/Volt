#include "sspch.h"

#include "SubSystem/SubSystemRequirementList.h"

void SubSystemRequirementList::RequiresIsTrue(bool value)
{
	Requirement& newRequirement = m_requirements.emplace_back();
	newRequirement.typeIndex = TypeTraits::TypeIndex::FromType<void>();
	newRequirement.requiredValue = value == true;
	newRequirement.resolveFunc = nullptr;
}
