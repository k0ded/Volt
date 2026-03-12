#include "rhipch.h"

#include "RHIModule/Graphics/Swapchain.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<Swapchain> Swapchain::Create(const SwapchainCreateInfo& createInfo)
	{
		return RHIModule::GetInstance().CreateSwapchain(createInfo);
	}
}
