#include "rhipch.h"

#include "RHIModule/Images/Image.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<Image> Image::Create(const ImageDesc& specification, const void* data, RefPtr<GPUAllocator> allocator)
	{
		return RHIModule::GetInstance().CreateImage(specification, data, allocator);
	}
	RefPtr<Image> Image::Create(const SwapchainImageDesc& specification)
	{
		return RHIModule::GetInstance().CreateImage(specification);
	}
}
