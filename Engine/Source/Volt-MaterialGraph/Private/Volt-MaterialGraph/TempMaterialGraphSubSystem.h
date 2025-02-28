#pragma once

#include <SubSystem/SubSystem.h>

namespace Volt
{
	class TempMaterialGraphSubSystem : public SubSystem
	{
	public:
		void Initialize() override;
	
		VT_DECLARE_SUBSYSTEM("{AD4305EB-E879-4447-9954-C2BBB1751C39}"_guid);
	};
}
