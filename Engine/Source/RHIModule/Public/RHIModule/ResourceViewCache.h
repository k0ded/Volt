#pragma once

#include "RHIModule/Images/ImageView.h"

#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>

class Image;

namespace Volt::RHI
{
	class ImageViewCache
	{
	public:
		VTRHI_API ImageViewCache(RawPtr<Image> image);
		VTRHI_API ~ImageViewCache();
		VTRHI_API IntRef<ImageView> GetOrCreateView(const ImageViewDesc& desc);

	private:
		struct ViewContainer
		{
			size_t hash;
			IntRef<RHI::ImageView> view;
			std::atomic<ViewContainer*> next = nullptr;
		};

		RawPtr<Image> m_image;

		std::atomic<ViewContainer*> m_rootView = nullptr;
		PagedAtomicArenaAllocator<ViewContainer, 8> m_views;
	};
}
