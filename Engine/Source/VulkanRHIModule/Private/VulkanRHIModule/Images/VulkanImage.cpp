#include "vkpch.h"
#include "VulkanRHIModule/Images/VulkanImage.h"

#include "VulkanRHIModule/Common/VulkanHelpers.h"
#include "VulkanRHIModule/Common/VulkanFunctions.h"
#include "VulkanRHIModule/Graphics/VulkanSwapchain.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Buffers/CommandBufferUtility.h>
#include <RHIModule/Memory/Allocation.h>
#include <RHIModule/Images/ImageView.h>
#include <RHIModule/Buffers/Buffer.h>

#include <RHIModule/Utility/ResourceUtility.h>

#include <RHIModule/RHIModule.h>

#include <CoreUtilities/Profiling/Profiling.h>

#include <vulkan/vulkan.h>
#include <CoreUtilities/MemoryUtility.h>

namespace Volt::RHI
{
	VulkanImage::VulkanImage(const ImageDesc& desc, const void* data)
		: m_desc(desc), m_viewCache(this)
	{
		VT_PROFILE_FUNCTION();

		Invalidate(desc.width, desc.height, desc.depth, data);
		SetName(desc.debugName);
	}

	VulkanImage::VulkanImage(const SwapchainImageDesc& desc)
		: m_isSwapchainImage(true), m_viewCache(this)
	{
		m_resourceStateTracker.Initialize(this, BarrierStage::None, BarrierAccess::None, ImageLayout::Undefined);

		InvalidateSwapchainImage(desc);
		m_desc.debugName = FormatString("Swapchain Image {}", desc.imageIndex);
		SetName(m_desc.debugName);
	}

	VulkanImage::~VulkanImage()
	{
		Release();
	}

	void VulkanImage::Invalidate(const uint32_t width, const uint32_t height, const uint32_t depth, const void* data)
	{
		Release();

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

		m_desc.width = width;
		m_desc.height = height;
		m_desc.depth = depth;

		if (m_desc.imageType == ResourceType::Image3D)
		{
			VT_ENSURE_MSG(m_desc.usage != ImageUsage::Attachment && m_desc.usage != ImageUsage::AttachmentStorage, "Attachment types are not supported for 3D images!");
		}

		m_allocation = GraphicsContext::GetDefaultAllocator()->CreateImage(m_desc, m_desc.memoryUsage);
		VT_ENSURE(m_allocation);

		ImageLayout targetLayout = ImageLayout::Undefined;

		switch (m_desc.usage)
		{
			case ImageUsage::Attachment:
			case ImageUsage::AttachmentStorage:
			{
				if ((GetImageAspect() & ImageAspect::Depth) != ImageAspect::None)
				{
					targetLayout = ImageLayout::DepthStencilWrite;
				}
				else
				{
					targetLayout = ImageLayout::RenderTarget;
				}
				break;
			}

			case ImageUsage::Texture:
			{
				targetLayout = ImageLayout::ShaderRead;
				break;
			}

			case ImageUsage::Storage:
			{
				targetLayout = ImageLayout::ShaderWrite;
				break;
			}
		}

		m_resourceStateTracker.Initialize(this, BarrierStage::None, BarrierAccess::None, ImageLayout::Undefined);

		if (data)
		{
			InitializeWithData(data);
		}

		if (m_desc.initializeImage)
		{
			TransitionToLayout(targetLayout);
		}
	}

	void VulkanImage::Release()
	{
		if (!m_allocation)
		{
			return;
		}

		GraphicsContext::GetDefaultAllocator()->DestroyImage(m_allocation);
		m_allocation = nullptr;
	}

	IntRef<ImageView> VulkanImage::GetView(const ImageViewDesc& desc)
	{
		VT_PROFILE_FUNCTION();

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

	void VulkanImage::SetName(const String& name)
	{
		if (Volt::RHI::vkSetDebugUtilsObjectNameEXT)
		{
			VkDebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;

			if (m_isSwapchainImage)
			{
				nameInfo.objectHandle = (uint64_t)m_swapchainImageData.image;
			}
			else
			{
				nameInfo.objectHandle = (uint64_t)m_allocation->GetResourceHandle<VkImage>();
			}

			nameInfo.pObjectName = name.c_str();

			auto device = GraphicsContext::GetDevice();
			Volt::RHI::vkSetDebugUtilsObjectNameEXT(device->GetHandle<VkDevice>(), &nameInfo);
		}

		m_desc.debugName = name;
	}

	StringView VulkanImage::GetName() const
	{
		return m_desc.debugName;
	}

	uint64_t VulkanImage::GetDeviceAddress() const
	{
		return m_allocation->GetDeviceAddress();
	}

	void* VulkanImage::GetHandleImpl() const
	{
		if (m_isSwapchainImage)
		{
			return m_swapchainImageData.image;
		}
		else
		{
			return m_allocation->GetResourceHandle<VkImage>();
		}
	}

	void VulkanImage::InvalidateSwapchainImage(const SwapchainImageDesc& specification)
	{
		const auto& vulkanSwapchain = specification.swapchain->AsRef<VulkanSwapchain>();

		m_imageAspect = ImageAspect::Color;
		m_desc.width = vulkanSwapchain.GetWidth();
		m_desc.height = vulkanSwapchain.GetHeight();
		m_desc.format = vulkanSwapchain.GetFormat();
		m_desc.usage = ImageUsage::Attachment;
		
		m_swapchainImageData.image = vulkanSwapchain.GetImageAtIndex(specification.imageIndex);
		m_swapchainImageData.swapchain = specification.swapchain;
	}

	void VulkanImage::TransitionToLayout(ImageLayout targetLayout)
	{
		IntRef<CommandBuffer> commandBuffer = CommandBuffer::Create();
		commandBuffer->Begin();

		RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsImageBarrier();
		ResourceUtility::InitializeBarrierSrcFromCurrentState(barrier.imageBarrier(), this);

		if (targetLayout == ImageLayout::ShaderRead)
		{
			barrier.imageBarrier().dstAccess = RHI::BarrierAccess::ShaderRead;
			barrier.imageBarrier().dstStage = RHI::BarrierStage::AllGraphics;
		}
		else if (targetLayout == ImageLayout::ShaderWrite)
		{
			barrier.imageBarrier().dstAccess = RHI::BarrierAccess::ShaderWrite;
			barrier.imageBarrier().dstStage = RHI::BarrierStage::ComputeShader | RHI::BarrierStage::PixelShader;
		}
		else if (targetLayout == ImageLayout::DepthStencilWrite)
		{
			barrier.imageBarrier().dstAccess = RHI::BarrierAccess::DepthStencilWrite;
			barrier.imageBarrier().dstStage = RHI::BarrierStage::DepthStencil;
		}
		else if (targetLayout == ImageLayout::RenderTarget)
		{
			barrier.imageBarrier().dstAccess = RHI::BarrierAccess::RenderTarget;
			barrier.imageBarrier().dstStage = RHI::BarrierStage::RenderTarget;
		}

		barrier.imageBarrier().dstLayout = targetLayout;
		barrier.imageBarrier().resource = this;

		commandBuffer->ResourceBarrier({ barrier });

		commandBuffer->End();

		IntRef<Fence> fence = CommandBufferUtils::ExecuteCommandBufferWithNewFence(commandBuffer);
		fence->WaitUntilSignaled();
	}

	void VulkanImage::InitializeWithData(const void* data)
	{
		// #TODO_Ivar: Implement correct size for layer + mip
		const VkDeviceSize bufferSize = m_desc.width * m_desc.height * Utility::GetByteSizePerPixelFromFormat(m_desc.format) * m_desc.layers;

		BufferDesc stagingDesc{};
		stagingDesc.numElements = bufferSize;
		stagingDesc.elementSize = 1;
		stagingDesc.usage = BufferUsage::TransferSrc;
		stagingDesc.memoryUsage = MemoryUsage::CPUToGPU;
		stagingDesc.debugName = "Staging Alloc";

		IntRef<Buffer> stagingBuffer = Buffer::Create(stagingDesc);

		auto* stagingData = stagingBuffer->Map<void>();
		memcpy_s(stagingData, bufferSize, data, bufferSize);
		stagingBuffer->Unmap();

		IntRef<CommandBuffer> commandBuffer = CommandBuffer::Create();

		commandBuffer->Begin();

		{
			RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsImageBarrier();
			ResourceUtility::InitializeBarrierSrcFromCurrentState(barrier.imageBarrier(), this);

			barrier.imageBarrier().dstStage = RHI::BarrierStage::Copy; 
			barrier.imageBarrier().dstAccess = RHI::BarrierAccess::CopyDest;
			barrier.imageBarrier().dstLayout = RHI::ImageLayout::CopyDest;
			barrier.imageBarrier().resource = this;

			commandBuffer->ResourceBarrier({ barrier });
		}

		commandBuffer->CopyBufferToImage(stagingBuffer, this, m_desc.width, m_desc.height, m_desc.depth);

		{
			RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsImageBarrier();
			ResourceUtility::InitializeBarrierSrcFromCurrentState(barrier.imageBarrier(), this);

			barrier.imageBarrier().dstStage = RHI::BarrierStage::PixelShader | RHI::BarrierStage::ComputeShader;
			barrier.imageBarrier().dstAccess = RHI::BarrierAccess::ShaderRead;
			barrier.imageBarrier().dstLayout = RHI::ImageLayout::ShaderRead;
			barrier.imageBarrier().resource = this;

			commandBuffer->ResourceBarrier({ barrier });
		}

		commandBuffer->End();

		IntRef<Fence> fence = CommandBufferUtils::ExecuteCommandBufferWithNewFence(commandBuffer);
		fence->WaitUntilSignaled();
	}

	const MemoryRequirement& VulkanImage::GetMemoryRequirements() const
	{
		return m_allocation->GetMemoryRequirements();
	}

	uint64_t VulkanImage::GetResourceByteSize() const
	{
		return m_allocation->GetMemoryRequirements().size;
	}
}
