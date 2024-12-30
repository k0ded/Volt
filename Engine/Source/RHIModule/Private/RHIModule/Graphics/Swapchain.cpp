#include "rhipch.h"

#include "RHIModule/Graphics/Swapchain.h"
#include "RHIModule/RHIProxy.h"

namespace Volt::RHI
{
	RefPtr<Swapchain> Swapchain::Create(const SwapchainCreateInfo& createInfo)
	{
		return RHIProxy::GetInstance().CreateSwapchain(createInfo);
	}
}
