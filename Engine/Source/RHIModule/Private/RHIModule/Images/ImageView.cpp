#include "rhipch.h"

#include "RHIModule/Images/ImageView.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	IntRef<ImageView> ImageView::Create(const ImageViewDesc& specification, RawPtr<Image> image)
	{
		return RHIModule::GetInstance().CreateImageView(specification, image);
	}
}
