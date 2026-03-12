#pragma once

#include <CoreUtilities/TypeTraits/TypeIndex.h>

#include <functional>

struct ECSEnvironmentDefinition
{
	std::function<void(void* dataPtr)> construct;
	std::function<void(void* dataPtr)> destruct;
	TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<void>();
	size_t typeSize;
};
