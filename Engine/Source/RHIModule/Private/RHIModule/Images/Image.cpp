#include "rhipch.h"

#include "RHIModule/Images/Image.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	IntRef<Image> Image::Create(const ImageDesc& specification, const void* data)
	{
		return RHIModule::GetInstance().CreateImage(specification, data);
	}
	IntRef<Image> Image::Create(const SwapchainImageDesc& specification)
	{
		return RHIModule::GetInstance().CreateImage(specification);
	}
}
