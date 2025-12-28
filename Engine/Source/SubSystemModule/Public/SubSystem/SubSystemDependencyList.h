#pragma once

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/VoltGUID.h>

class SubSystemDependencyList
{
public:
	template<typename T>
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
