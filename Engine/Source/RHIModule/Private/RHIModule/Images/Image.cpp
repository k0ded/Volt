#include "rhipch.h"

#include "RHIModule/Images/Image.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<Image> Image::Create(const ImageDesc& specification, const void* data)
	{
		return RHIModule::GetInstance().CreateImage(specification, data);
	}
	RefPtr<Image> Image::Create(const SwapchainImageDesc& specification)
	{
		return RHIModule::GetInstance().CreateImage(specification);
	}
}
