#include "vkpch.h"
#include "VulkanRHIModule/Images/VulkanImageView.h"

#include "VulkanRHIModule/Common/VulkanHelpers.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"

#include <RHIModule/Images/Image.h>
#include <RHIModule/Images/ImageUtility.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/GraphicsDevice.h>

#include <RHIModule/RHIModule.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanImageView::VulkanImageView(const ImageViewDesc& desc)
		: m_desc(desc)
	{
		// Keep a reference to the image, as it should be alive untill all views have been destroyed.
		desc.image->IncRef();

		auto imageRes = desc.image;
		auto image = imageRes->As<Image>();

		m_format = image->GetFormat();
		m_imageUsage = image->GetUsage();
		m_imageAspect = image->GetImageAspect();
		m_isSwapchainImage = image->IsSwapchainImage();

		VkImageAspectFlags aspectMask = Utility::IsDepthFormat(m_format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		if (Utility::IsStencilFormat(m_format))
		{
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.viewType = Utility::VoltToVulkanViewType(desc.viewType);
		viewInfo.format = Utility::VoltToVulkanFormat(m_format);
		viewInfo.flags = 0;
		viewInfo.subresourceRange = {};
		viewInfo.subresourceRange.aspectMask = aspectMask;
		viewInfo.subresourceRange.baseMipLevel = desc.baseMipLevel;
		viewInfo.subresourceRange.baseArrayLayer = desc.baseArrayLayer;
		viewInfo.subresourceRange.levelCount = desc.mipCount == ImageViewDesc::MipCountMax ? image->GetMipCount() : desc.mipCount;
		viewInfo.subresourceRange.layerCount = desc.layerCount == ImageViewDesc::LayerCountMax ? image->GetLayerCount() : desc.layerCount;
		viewInfo.image = image->GetHandle<VkImage>();

		auto device = GraphicsContext::GetDevice();
		VT_VK_CHECK(vkCreateImageView(device->GetHandle<VkDevice>(), &viewInfo, VT_VULKAN_ALLOCATOR, &m_imageView));
	}

	VulkanImageView::~VulkanImageView()
	{
		// Remove the reference we add on creation.
		m_desc.image->DecRef();

		RHIModule::GetInstance().DestroyResource([imageView = m_imageView]()
		{
			auto device = GraphicsContext::GetDevice();
			vkDestroyImageView(device->GetHandle<VkDevice>(), imageView, VT_VULKAN_ALLOCATOR);
		});

		m_imageView = nullptr;
	}

	const PixelFormat VulkanImageView::GetFormat() const
	{
		return m_format;
	}

	const ImageAspect VulkanImageView::GetImageAspect() const
	{
		return m_imageAspect;
	}

	const uint64_t VulkanImageView::GetDeviceAddress() const
	{
		return m_desc.image->GetDeviceAddress();
	}

	const ImageUsage VulkanImageView::GetImageUsage() const
	{
		return m_imageUsage;
	}

	const ImageViewType VulkanImageView::GetViewType() const
	{
		return m_desc.viewType;
	}

	const bool VulkanImageView::IsSwapchainView() const
	{
		return m_isSwapchainImage;
	}

	void* VulkanImageView::GetHandleImpl() const
	{
		return m_imageView;
	}

	const ImageViewDesc& VulkanImageView::GetDesc() const
	{
		return m_desc;
	}
}
