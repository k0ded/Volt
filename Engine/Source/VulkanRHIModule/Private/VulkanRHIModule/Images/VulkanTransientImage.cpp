#include "vkpch.h"

#include "VulkanRHIModule/Images/VulkanTransientImage.h"
#include "VulkanRHIModule/Common/VulkanFunctions.h"
#include "VulkanRHIModule/Common/VulkanHelpers.h"
#include "VulkanRHIModule/Common/VulkanCPUAllocator.h"
#include "VulkanRHIModule/VulkanResourceCast.h"

#include <RHIModule/Images/ImageUtility.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Memory/Allocation.h>
#include <RHIModule/RHIModule.h>

#include <CoreUtilities/EnumUtils.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanTransientImage::VulkanTransientImage(const ImageDesc& desc)
		: m_desc(desc),
		m_viewCache(this),
		m_imageHandle(nullptr)
	{
		CreateImage();
	}

	VulkanTransientImage::~VulkanTransientImage()
	{
		if (m_imageHandle)
		{
			RHIModule::GetInstance().DestroyResource([imageHandle = m_imageHandle]()
			{
				auto device = GraphicsContext::GetDevice();
				vkDestroyImage(device->GetHandle<VkDevice>(), imageHandle, VT_VULKAN_ALLOCATOR);
			}, GetLastSubmissionTrackerFence());
		}
	}

	IntRef<ImageView> VulkanTransientImage::GetView(const ImageViewDesc& desc)
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

	void VulkanTransientImage::SetName(const String& name)
	{
		if (Volt::RHI::vkSetDebugUtilsObjectNameEXT)
		{
			VkDebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
			nameInfo.objectHandle = (uint64_t)m_imageHandle;
			nameInfo.pObjectName = name.c_str();

			auto device = GraphicsContext::GetDevice();
			Volt::RHI::vkSetDebugUtilsObjectNameEXT(device->GetHandle<VkDevice>(), &nameInfo);
		}

		m_desc.debugName = name;
	}

	StringView VulkanTransientImage::GetName() const
	{
		return m_desc.debugName;
	}

	uint64_t VulkanTransientImage::GetDeviceAddress() const
	{
		return m_deviceAddress;
	}

	const MemoryRequirement& VulkanTransientImage::GetMemoryRequirements() const
	{
		return m_memoryRequirements;
	}

	void* VulkanTransientImage::GetHandleImpl() const
	{
		return m_imageHandle;
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

		{
			auto device = GraphicsContext::GetDevice();

			const VkImageCreateInfo vkImageInfo = Utility::GetVkImageCreateInfo(m_desc);
			m_memoryRequirements = Utility::GetImageMemoryRequirement(vkImageInfo);

			vkCreateImage(device->GetHandle<VkDevice>(), &vkImageInfo, VT_VULKAN_ALLOCATOR, &m_imageHandle);
		}

		m_resourceStateTracker.Initialize(this, BarrierStage::None, BarrierAccess::None);
	}

	uint64_t VulkanTransientImage::GetResourceByteSize() const
	{
		return m_memoryRequirements.size;
	}

	void VulkanTransientImage::BindMemory(IntRef<RHI::TransientHeap> heap, uint32_t pageIndex, uint64_t offset)
	{
		auto device = GraphicsContext::GetDevice();

		IntRef<RHI::VulkanTransientHeap> vkHeap = ResourceCast(heap);
		vkBindImageMemory(device->GetHandle<VkDevice>(), m_imageHandle, vkHeap->GetPageMemoryHandle(pageIndex), offset);
	}
}
