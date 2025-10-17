#pragma once

#include "VulkanRHIModule/Core.h"
#include <RHIModule/Images/ImageView.h>
	
struct VkImageView_T;

namespace Volt::RHI
{
	class VulkanImageView final : public ImageView
	{
	public:
		VulkanImageView(const ImageViewDesc& specification, RawPtr<Image> image);
		~VulkanImageView() override;

		const PixelFormat GetFormat() const;
		const ImageAspect GetImageAspect() const override;
		const uint64_t GetDeviceAddress() const override;
		const ImageUsage GetImageUsage() const override;
		const ImageViewType GetViewType() const override;
		const ImageViewDesc& GetDesc() const override;
		RawPtr<Image> GetImage() const override;
		const bool IsSwapchainView() const override;

	protected:
		void* GetHandleImpl() const override;

	private:
		ImageViewDesc m_desc{};

		VkImageView_T* m_imageView = nullptr;
		RawPtr<Image> m_image;

		PixelFormat m_format;
		ImageAspect m_imageAspect;
		ImageUsage m_imageUsage;
		bool m_isSwapchainImage;
	};
}
