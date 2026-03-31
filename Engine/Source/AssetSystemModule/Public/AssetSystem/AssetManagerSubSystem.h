#pragma once

#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

namespace Volt
{
	class AssetManagerSubSystem : public SubSystem
	{
	public:
		void Initialize() override;
		void Shutdown() override;

		static void GetSubSystemDependencies(SubSystemDependencyList& outDependencies);
		VT_DECLARE_SUBSYSTEM("{B098AAA9-FD87-48E8-ACDE-6F720027C16E}"_guid);
	};
}
