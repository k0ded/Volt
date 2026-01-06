#include "vkpch.h"
#include "VulkanRHIModule/Images/VulkanImageView.h"

#include "VulkanRHIModule/Common/VulkanHelpers.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"

#include "VulkanRHIModule/Graphics/PhysicalDeviceProperties.h"

#include <RHIModule/Images/Image.h>
#include <RHIModule/Images/ImageUtility.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/GraphicsDevice.h>

#include <RHIModule/RHIModule.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanImageView::VulkanImageView(const ImageViewDesc& desc, RawPtr<Image> image)
		: m_desc(desc), m_image(image)
	{
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

		CreateDescriptors();
	}

	VulkanImageView::~VulkanImageView()
	{
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
		return m_image->GetDeviceAddress();
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

	RawPtr<Image> VulkanImageView::GetImage() const
	{
		return m_image;
	}

	void VulkanImageView::CreateDescriptors()
	{
		memset(&m_srvDescriptor, 0, sizeof(m_srvDescriptor));
		memset(&m_uavDescriptor, 0, sizeof(m_uavDescriptor));

		m_srvDescriptor.vkDescriptorInfo.sType = m_uavDescriptor.vkDescriptorInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT;
		m_srvDescriptor.vkDescriptorInfo.pNext = m_uavDescriptor.vkDescriptorInfo.pNext = nullptr;

		m_srvDescriptor.vkDescriptorInfo.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		m_srvDescriptor.vkDescriptorInfo.data.pSampledImage = &m_srvDescriptor.vkImageDescriptor;

		m_uavDescriptor.vkDescriptorInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		m_uavDescriptor.vkDescriptorInfo.data.pStorageImage = &m_uavDescriptor.vkImageDescriptor;

		m_srvDescriptor.vkImageDescriptor.imageView = m_imageView;
		m_srvDescriptor.vkImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		m_uavDescriptor.vkImageDescriptor.imageView = m_imageView;
		m_uavDescriptor.vkImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		m_srvDescriptor.descriptorSize = g_physicalDeviceProperties.descriptorBufferProperties.sampledImageDescriptorSize;
		m_uavDescriptor.descriptorSize = g_physicalDeviceProperties.descriptorBufferProperties.storageImageDescriptorSize;
	}
}
