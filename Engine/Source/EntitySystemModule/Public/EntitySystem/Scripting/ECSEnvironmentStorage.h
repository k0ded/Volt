#pragma once

#include "EntitySystem/Scripting/ECSEnvironmentDefinition.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Buffer/Buffer.h>
#include <CoreUtilities/TypeTraits/TypeIndex.h>

struct ECSEnvironmentDefinition;

class ECSEnvironmentStorage
{
public:
	~ECSEnvironmentStorage();

	void InitializeWith(const Vector<ECSEnvironmentDefinition>& environmentDefinitions);
	void Clear();

	VT_NODISCARD VT_INLINE bool ContainsType(TypeTraits::TypeIndex typeIndex) const { return m_environmentTypeIndexToEnvData.contains(typeIndex); }
	VT_NODISCARD VT_INLINE const void* GetDataPointerOfType(TypeTraits::TypeIndex typeIndex) const { return m_environmentTypeIndexToEnvData.at(typeIndex).dataPtr; }
	VT_NODISCARD VT_INLINE void* GetDataPointerOfType(TypeTraits::TypeIndex typeIndex) { return m_environmentTypeIndexToEnvData.at(typeIndex).dataPtr; }

private:
	struct EnvironmentData
	{
		void* dataPtr;
		ECSEnvironmentDefinition definition;
	};

	Buffer m_dataBuffer;
	Map<TypeTraits::TypeIndex, EnvironmentData> m_environmentTypeIndexToEnvData;
};
