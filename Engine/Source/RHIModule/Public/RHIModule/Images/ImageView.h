#pragma once

#include "RHIModule/Descriptors/BindlessIndex.h"

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Core/RHICommon.h"

namespace Volt::RHI
{
	class Image;

	struct ImageViewDesc
	{
		inline static constexpr uint32_t LayerCountMax = 31;
		inline static constexpr uint32_t MipCountMax = 31;

		ImageViewType viewType = ImageViewType::View2D;

		uint32_t baseMipLevel = 0;
		uint32_t baseArrayLayer = 0;
		uint32_t mipCount = MipCountMax;
		uint32_t layerCount = LayerCountMax;
	};

	class VTRHI_API ImageView : public ArenaRHIInterface
	{
	public:
		static IntRef<ImageView> Create(const ImageViewDesc& specification, RawPtr<Image> image);

		virtual PixelFormat GetFormat() const = 0;
		virtual ImageAspect GetImageAspect() const = 0;
		virtual ImageUsage GetImageUsage() const = 0;
		virtual ImageViewType GetViewType() const = 0;
		virtual const ImageViewDesc& GetDesc() const = 0;
		virtual RawPtr<Image> GetImage() const = 0;
		virtual bool IsSwapchainView() const = 0;

		virtual uint64_t GetDeviceAddress() const = 0;
		virtual BindlessIndex GetSRVBindlessIndex() const = 0;
		virtual BindlessIndex GetUAVBindlessIndex() const = 0;

	protected:
		ImageView() = default;
		~ImageView() override = default;
	};
}
