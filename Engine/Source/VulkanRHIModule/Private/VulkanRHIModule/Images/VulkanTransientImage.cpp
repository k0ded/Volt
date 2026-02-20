#include "vkpch.h"

#include "VulkanRHIModule/Images/VulkanTransientImage.h"
#include "VulkanRHIModule/Common/VulkanFunctions.h"

#include <RHIModule/Images/ImageUtility.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Memory/Allocation.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanTransientImage::VulkanTransientImage(const ImageDesc& desc)
		: m_desc(desc),
		m_viewCache(this)
	{
		CreateImage();
	}

	VulkanTransientImage::~VulkanTransientImage()
	{
		if (m_allocation)
		{
			GraphicsContext::GetTransientAllocator()->DestroyImage(m_allocation);
		}
		GraphicsContext::GetResourceStateTracker()->RemoveResource(this);
	}

	RefPtr<ImageView> VulkanTransientImage::GetView(const ImageViewDesc& desc)
	{
		ImageViewDesc tempDesc = desc;

		if (desc.layerCount > 1 && m_desc.layers > 1 && tempDesc.viewType == ImageViewType::View2D)
		{
			tempDesc.viewType = ImageViewType::View2DArray;
		}

		if (tempDesc.viewType == ImageViewType::View1D)
		{
			VT_ENSURE(m_desc.imageType == ResourceType::Image1D);
		}
		else if (tempDesc.viewType == ImageViewType::View1DArray)
		{
			VT_ENSURE(m_desc.imageType == ResourceType::Image1D && m_desc.layers > 1);
		}
		else if (tempDesc.viewType == ImageViewType::View2D)
		{
			VT_ENSURE(m_desc.imageType == ResourceType::Image2D);
		}
		else if (tempDesc.viewType == ImageViewType::View2DArray)
		{
			VT_ENSURE(m_desc.imageType == ResourceType::Image2D && m_desc.layers > 1);
		}
		else if (tempDesc.viewType == ImageViewType::View3D)
		{
			VT_ENSURE(m_desc.imageType == ResourceType::Image3D);
		}
		else if (tempDesc.viewType == ImageViewType::View3DArray)
		{
			VT_ENSURE(m_desc.imageType == ResourceType::Image3D && m_desc.layers > 1);
		}
		else if (tempDesc.viewType == ImageViewType::ViewCube)
		{
			VT_ENSURE(m_desc.imageType == ResourceType::Image2D && m_desc.isCubeMap && m_desc.layers % 6 == 0);
		}

		if (tempDesc.viewType == ImageViewType::ViewCube)
		{
			tempDesc.layerCount = 6;

			// When using cube array, baseArrayLayer specifies which cubemap index
			if (m_desc.layers > 6 && tempDesc.baseArrayLayer > 0 && tempDesc.baseArrayLayer != ImageViewDesc::LayerCountMax)
			{
				tempDesc.baseArrayLayer = tempDesc.baseArrayLayer * 6;
			}
		}

		return m_viewCache.GetOrCreateView(tempDesc);
	}

	void VulkanTransientImage::SetName(const std::string& name)
	{
		if (Volt::RHI::vkSetDebugUtilsObjectNameEXT)
		{
			VkDebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
			nameInfo.objectHandle = (uint64_t)m_allocation->GetResourceHandle<VkImage>();
			nameInfo.pObjectName = name.c_str();

			auto device = GraphicsContext::GetDevice();
			Volt::RHI::vkSetDebugUtilsObjectNameEXT(device->GetHandle<VkDevice>(), &nameInfo);
		}

		m_desc.debugName = name;
	}

	std::string_view VulkanTransientImage::GetName() const
	{
		return m_desc.debugName;
	}

	uint64_t VulkanTransientImage::GetDeviceAddress() const
	{
		return m_allocation->GetDeviceAddress();
	}

	const MemoryRequirement& VulkanTransientImage::GetMemoryRequirements() const
	{
		return m_allocation->GetMemoryRequirements();
	}

	void* VulkanTransientImage::GetHandleImpl() const
	{
		return m_allocation->GetResourceHandle<VkImage>();
	}

	void VulkanTransientImage::CreateImage()
	{
		if (Utility::IsDepthFormat(m_desc.format))
		{
			m_imageAspect = ImageAspect::Depth;

			if (Utility::IsStencilFormat(m_desc.format))
			{
				m_imageAspect |= ImageAspect::Stencil;
			}
		}
		else
		{
			m_imageAspect = ImageAspect::Color;
		}

		if (m_desc.imageType == ResourceType::Image3D)
		{
			VT_ENSURE_MSG(m_desc.usage != ImageUsage::Attachment && m_desc.usage != ImageUsage::AttachmentStorage, "Attachment types are not supported for 3D images!");
		}

		m_allocation = GraphicsContext::GetTransientAllocator()->CreateImage(m_desc, m_desc.memoryUsage);
		VT_ENSURE(m_allocation);

		GraphicsContext::GetResourceStateTracker()->AddResource(this, BarrierStage::None, BarrierAccess::None, ImageLayout::Undefined);
	}
}
