#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Core/RHICommon.h"

namespace Volt::RHI
{
	struct ImageViewDesc
	{
		inline static constexpr uint32_t LayerCountMax = 31;
		inline static constexpr uint32_t MipCountMax = 31;

		ImageViewType viewType = ImageViewType::View2D;

		uint32_t baseMipLevel = 0;
		uint32_t baseArrayLayer = 0;
		uint32_t mipCount = MipCountMax;
		uint32_t layerCount = LayerCountMax;

		RawPtr<RHIResource> image = nullptr;
	};

	class VTRHI_API ImageView : public ArenaRHIInterface
	{
	public:
		static RefPtr<ImageView> Create(const ImageViewDesc& specification);

		virtual const ImageAspect GetImageAspect() const = 0;
		virtual const uint64_t GetDeviceAddress() const = 0;
		virtual const ImageUsage GetImageUsage() const = 0;
		virtual const ImageViewType GetViewType() const = 0;
		virtual const ImageViewDesc& GetDesc() const = 0;
		virtual const bool IsSwapchainView() const = 0;

	protected:
		ImageView() = default;
		~ImageView() override = default;
	};
}
