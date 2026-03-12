#include "rhipch.h"

#include "RHIModule/ResourceViewCache.h"

#include <CoreUtilities/Math/Hash.h>
#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt::RHI
{
	namespace Utility
	{
		VT_INLINE size_t GetHashFromImageViewDesc(const RHI::ImageViewDesc& desc)
		{
			size_t hash = Math::HashCombine(std::hash<uint32_t>()(static_cast<uint32_t>(desc.viewType)), std::hash<uint32_t>()(desc.baseMipLevel));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(desc.baseArrayLayer));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(desc.mipCount));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(desc.layerCount));

			return hash;
		}
	}

	ImageViewCache::ImageViewCache(RawPtr<Image> image)
		: m_image(image)
	{

	}

	RefPtr<ImageView> ImageViewCache::GetOrCreateView(const ImageViewDesc& desc)
	{
		VT_PROFILE_FUNCTION();

		const size_t hash = Utility::GetHashFromImageViewDesc(desc);

		if (m_rootView.load(std::memory_order::acquire) == nullptr)
		{
			ViewContainer* newView = m_views.Allocate();
			newView->view = ImageView::Create(desc, m_image);
			newView->hash = hash;

			ViewContainer* expected = nullptr;
			if (!m_rootView.compare_exchange_strong(expected, newView, std::memory_order::release, std::memory_order::acquire))
			{
				m_views.Free(newView);
			}
		}

		ViewContainer* currentView = m_rootView.load(std::memory_order::acquire);

		while (true)
		{
			// Check if view is of wanted type.
			if (currentView->hash == hash)
			{
				return currentView->view;
			}
			else
			{
				// Otherwise we go the the next one, or allocate a new
				if (currentView->next.load(std::memory_order::acquire) == nullptr)
				{
					ViewContainer* newView = m_views.Allocate();
					newView->view = ImageView::Create(desc, m_image);
					newView->hash = hash;

					ViewContainer* expected = nullptr;
					if (!currentView->next.compare_exchange_strong(expected, newView, std::memory_order::release, std::memory_order::acquire))
					{
						m_views.Free(newView);
					}
				}

				currentView = currentView->next.load(std::memory_order::acquire);
			}
		}
	}

	ImageViewCache::~ImageViewCache()
	{

	}
}
