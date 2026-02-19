#pragma once

#include "RHIModule/Core/RHIResource.h"
#include "RHIModule/Memory/GPUAllocator.h"

#include "RHIModule/Images/ImageView.h"

#include <CoreUtilities/Buffer/DataBuffer.h>

namespace Volt::RHI
{
	class Swapchain;

	class Image : public RHIResource
	{
	public:
		virtual RefPtr<ImageView> GetView(const ImageViewDesc& desc = {}) = 0;
		virtual const ImageAspect GetImageAspect() const = 0;

		virtual const uint32_t GetWidth() const = 0;
		virtual const uint32_t GetHeight() const = 0;
		virtual const uint32_t GetDepth() const = 0;

		virtual const bool IsSwapchainImage() const = 0;
		virtual const ImageDesc& GetDesc() const = 0;

		VTRHI_API static RefPtr<Image> Create(const ImageDesc& specification, const void* data = nullptr, RefPtr<GPUAllocator> allocator = nullptr);
		VTRHI_API static RefPtr<Image> Create(const SwapchainImageDesc& specification);

	protected:
		Image() = default;
		~Image() override = default;
	};
}
