#include "rhipch.h"

#include "RHIModule/Images/ImageView.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<ImageView> ImageView::Create(const ImageViewSpecification& specification)
	{
		return RHIModule::GetInstance().CreateImageView(specification);
	}
}
