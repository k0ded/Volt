#include "rhipch.h"

#include "RHIModule/Descriptors/BindlessDescriptorTable.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<BindlessDescriptorTable> BindlessDescriptorTable::Create(const uint64_t framesInFlight)
	{
		return RHIModule::GetInstance().CreateBindlessDescriptorTable(framesInFlight);
	}
}
