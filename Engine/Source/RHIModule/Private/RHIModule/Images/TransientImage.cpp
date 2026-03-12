#include "rhipch.h"

#include "RHIModule/Images/TransientImage.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<TransientImage> TransientImage::Create(const ImageDesc& desc)
	{
		return RHIModule::GetInstance().CreateTransientImage(desc);
	}
}
