#include "rhipch.h"

#include "RHIModule/Descriptors/DescriptorTable.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<DescriptorTable> DescriptorTable::Create2(const DescriptorTableCreateInfo& specification)
	{
		return RHIModule::GetInstance().CreateDescriptorTable2(specification);
	}
}
