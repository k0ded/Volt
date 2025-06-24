#include "rhipch.h"

#include "RHIModule/Images/ImageView.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<ImageView> ImageView::Create(const ImageViewDesc& specification)
	{
		return RHIModule::GetInstance().CreateImageView(specification);
	}
}
