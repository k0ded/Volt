#pragma once

#include "VulkanRHIModule/Core.h"
#include "VulkanRHIModule/LastSubmissionTracker.h"

#include <RHIModule/Images/ImageView.h>
	
#include <vulkan/vulkan.h>

struct VkImageView_T;

namespace Volt::RHI
{
	class VulkanImageView final : public ImageView, public LastSubmissionTracker
	{
	public:
		struct DescriptorDescription
		{
			VkDescriptorGetInfoEXT vkDescriptorInfo;
			VkDescriptorImageInfo vkImageDescriptor;
			uint64_t descriptorSize;
		};

		VulkanImageView(const ImageViewDesc& specification, RawPtr<Image> image);
		~VulkanImageView() override;

		PixelFormat GetFormat() const override;
		ImageAspect GetImageAspect() const override;
		ImageUsage GetImageUsage() const override;
		ImageViewType GetViewType() const override;
		const ImageViewDesc& GetDesc() const override;
		RawPtr<Image> GetImage() const override;
		bool IsSwapchainView() const override;

		uint64_t GetDeviceAddress() const override;
		BindlessIndex GetSRVBindlessIndex() const override;
		BindlessIndex GetUAVBindlessIndex() const override;

		VT_NODISCARD VT_INLINE const DescriptorDescription& GetSRVDescriptor() const { return m_srvDescriptor; }
		VT_NODISCARD VT_INLINE const DescriptorDescription& GetUAVDescriptor() const { return m_uavDescriptor; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void CreateDescriptors();

		ImageViewDesc m_desc{};

		VkImageView_T* m_imageView = nullptr;
		RawPtr<Image> m_image;

		BindlessIndex m_srvBindlessIndex;
		BindlessIndex m_uavBindlessIndex;

		PixelFormat m_format;
		ImageAspect m_imageAspect;
		ImageUsage m_imageUsage;
		bool m_isSwapchainImage;

		DescriptorDescription m_srvDescriptor;
		DescriptorDescription m_uavDescriptor;
	};
}
