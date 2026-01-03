#pragma once

#include "SubSystem/SubSystem.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/VoltGUID.h>

template<typename T>
concept SubSystemType = std::is_base_of_v<SubSystem, T>;

class SubSystemDependencyList
{
public:
	template<SubSystemType T>
	void AddDependency()
	{
		constexpr VoltGUID SubSystemGUID = T::GetStaticSubSystemGUID();
		if (!m_dependencyList.contains(SubSystemGUID))
		{
			m_dependencyList.emplace_back(SubSystemGUID);
		}
	}

	VT_INLINE const Vector<VoltGUID>& GetDependencyList() const { return m_dependencyList; }

private:
	Vector<VoltGUID> m_dependencyList;
};
