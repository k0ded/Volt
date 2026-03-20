#include "rhipch.h"

#include "RHIModule/Graphics/Swapchain.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	IntRef<Swapchain> Swapchain::Create(const SwapchainCreateInfo& createInfo)
	{
		return RHIModule::GetInstance().CreateSwapchain(createInfo);
	}
}
