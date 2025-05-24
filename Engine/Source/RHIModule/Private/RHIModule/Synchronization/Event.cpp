#include "rhipch.h"

#include "RHIModule/Synchronization/Event.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<Event> Event::Create(const EventCreateInfo& createInfo)
	{
		return RHIModule::GetInstance().CreateEvent(createInfo);
	}
}
