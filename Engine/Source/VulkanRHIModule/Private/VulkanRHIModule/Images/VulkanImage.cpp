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

#include <RHIModule/Utility/ResourceUtility.h>

#include <RHIModule/RHIModule.h>

#include <CoreUtilities/Profiling/Profiling.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanImage::VulkanImage(const ImageDesc& desc, const void* data, RefPtr<GPUAllocator> allocator)
		: m_desc(desc), m_allocator(allocator)
	{
		VT_PROFILE_FUNCTION();

		if (!allocator)
		{
			m_allocator = GraphicsContext::GetDefaultAllocator();
		}

		Invalidate(desc.width, desc.height, desc.depth, data);
		SetName(desc.debugName);
	}

	VulkanImage::VulkanImage(const SwapchainImageDesc& desc)
		: m_isSwapchainImage(true)
	{
		GraphicsContext::GetResourceStateTracker()->AddResource(this, BarrierStage::None, BarrierAccess::None, ImageLayout::Undefined);

		InvalidateSwapchainImage(desc);
		m_desc.debugName = std::format("Swapchain Image {}", desc.imageIndex);
		SetName(m_desc.debugName);
	}

	VulkanImage::~VulkanImage()
	{
		GraphicsContext::GetResourceStateTracker()->RemoveResource(this);
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

		m_allocation = m_allocator->CreateImage(m_desc, m_desc.memoryUsage);
		VT_ENSURE(m_allocation);

		ImageLayout targetLayout = ImageLayout::Undefined;

		if (m_desc.imageType == ResourceType::Image3D)
		{
			VT_ENSURE_MSG(m_desc.usage != ImageUsage::Attachment && m_desc.usage != ImageUsage::AttachmentStorage, "Attachment types are not supported for 3D images!");
		}

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

		GraphicsContext::GetResourceStateTracker()->AddResource(this, BarrierStage::None, BarrierAccess::None, ImageLayout::Undefined);

		if (data)
		{
			InitializeWithData(data);
		}

		if (m_desc.initializeImage)
		{
			TransitionToLayout(targetLayout);
		}

		if (m_desc.generateMips && m_desc.mips > 1)
		{
			GenerateMips();
		}
	}

	void VulkanImage::Release()
	{
		if (!m_allocation)
		{
			return;
		}

		m_allocator->DestroyImage(m_allocation);
		m_allocation = nullptr;
	}

	void VulkanImage::GenerateMips()
	{
		if (m_hasGeneratedMips || m_isSwapchainImage)
		{
			return;
		}

		const std::string markerName = std::format("Generate Mips {}", m_desc.debugName);

		RefPtr<CommandBuffer> commandBuffer = CommandBuffer::Create();
		commandBuffer->Begin();
		commandBuffer->BeginMarker(markerName, { 1.f, 1.f, 1.f, 1.f });

		VkCommandBuffer vkCmdBuffer = commandBuffer->GetHandle<VkCommandBuffer>();

		const auto& currentState = GraphicsContext::GetResourceStateTracker()->GetCurrentResourceState(this);

		// Transition to DST OPTIMAL
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = m_allocation->GetResourceHandle<VkImage>();
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.oldLayout = Utility::GetVkImageLayoutFromImageLayout(currentState.layout);
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.baseMipLevel = 0;

		const uint32_t mipLevels = CalculateMipCount();
		m_desc.mips = mipLevels;

		vkCmdPipelineBarrier(vkCmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		for (uint32_t i = 1; i < mipLevels; i++)
		{
			for (uint32_t layer = 0; layer < m_desc.layers; layer++)
			{
				// Transfer last mip
				{
					barrier.subresourceRange.baseMipLevel = i - 1;
					barrier.subresourceRange.baseArrayLayer = layer;
					barrier.subresourceRange.levelCount = 1;
					barrier.subresourceRange.layerCount = 1;

					barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

					vkCmdPipelineBarrier(vkCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
				}

				// Perform blit
				{
					VkImageBlit imageBlit{};
					imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageBlit.srcSubresource.layerCount = 1;
					imageBlit.srcSubresource.mipLevel = i - 1;
					imageBlit.srcSubresource.baseArrayLayer = layer;

					imageBlit.srcOffsets[0] = { 0, 0, 0 };
					imageBlit.srcOffsets[1] = { int32_t(m_desc.width >> (i - 1)), int32_t(m_desc.height >> (i - 1)), 1 };

					imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageBlit.dstSubresource.layerCount = 1;
					imageBlit.dstSubresource.mipLevel = i;
					imageBlit.dstSubresource.baseArrayLayer = layer;

					imageBlit.dstOffsets[0] = { 0, 0, 0 };
					imageBlit.dstOffsets[1] = { int32_t(m_desc.width >> i), int32_t(m_desc.height >> i), 1 };

					vkCmdBlitImage(vkCmdBuffer, m_allocation->GetResourceHandle<VkImage>(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_allocation->GetResourceHandle<VkImage>(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);
				}

				// Transfer last mip back
				{
					barrier.srcAccessMask = 0;
					barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

					vkCmdPipelineBarrier(vkCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
				}
			}
		}

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = Utility::GetVkImageLayoutFromImageLayout(currentState.layout);
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.baseMipLevel = 0;

		vkCmdPipelineBarrier(vkCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		commandBuffer->EndMarker();
		commandBuffer->End();

		RefPtr<Fence> fence = CommandBufferUtils::ExecuteCommandBufferWithNewFence(commandBuffer);
		fence->WaitUntilSignaled();

		m_hasGeneratedMips = true;
	}

	RefPtr<ImageView> VulkanImage::GetView(const ImageViewDesc& desc)
	{
		VT_PROFILE_FUNCTION();

		ImageViewDesc tempDesc = desc;
		tempDesc.image = this;

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

		return ImageView::Create(tempDesc);;
	}

	const uint32_t VulkanImage::CalculateMipCount() const
	{
		return Utility::CalculateMipCount(m_desc.width, m_desc.height);
	}

	void VulkanImage::SetName(const std::string& name)
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

	std::string_view VulkanImage::GetName() const
	{
		return m_desc.debugName;
	}

	const uint64_t VulkanImage::GetDeviceAddress() const
	{
		return m_allocation->GetDeviceAddress();
	}

	const uint64_t VulkanImage::GetByteSize() const
	{
		return m_allocation->GetSize();
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

	Buffer VulkanImage::ReadPixelInternal(const uint32_t x, const uint32_t y, const uint32_t z, const size_t stride)
	{
		// #TODO_Ivar: Implement correct size for layer + mip
		const VkDeviceSize bufferSize = m_desc.width * m_desc.height * Utility::GetByteSizePerPixelFromFormat(m_desc.format) * m_desc.layers;

		BufferDesc stagingDesc{};
		stagingDesc.count = 1;
		stagingDesc.elementSize = bufferSize;
		stagingDesc.usage = BufferUsage::TransferDst;
		stagingDesc.memoryUsage = MemoryUsage::GPUToCPU;
		stagingDesc.debugName = "Staging Alloc";

		Handle<Allocation> stagingAlloc = GraphicsContext::GetDefaultAllocator()->CreateBuffer(stagingDesc);

		VkImageAspectFlags aspectFlags = Utility::IsDepthFormat(m_desc.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		if (Utility::IsStencilFormat(m_desc.format))
		{
			aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		const auto& currentState = GraphicsContext::GetResourceStateTracker()->GetCurrentResourceState(this);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = m_allocation->GetResourceHandle<VkImage>();
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.oldLayout = Utility::GetVkImageLayoutFromImageLayout(currentState.layout);
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.subresourceRange.aspectMask = Utility::GetVkImageAspect(m_imageAspect);
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.baseMipLevel = 0;

		RefPtr<CommandBuffer> commandBuffer = CommandBuffer::Create();

		commandBuffer->Begin();
		vkCmdPipelineBarrier(commandBuffer->GetHandle<VkCommandBuffer>(), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = aspectFlags;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { m_desc.width, m_desc.height, m_desc.depth };

		vkCmdCopyImageToBuffer(commandBuffer->GetHandle<VkCommandBuffer>(), m_allocation->GetResourceHandle<VkImage>(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingAlloc->GetResourceHandle<VkBuffer>(), 1, &region);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = Utility::GetVkImageLayoutFromImageLayout(currentState.layout);
		vkCmdPipelineBarrier(commandBuffer->GetHandle<VkCommandBuffer>(), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		commandBuffer->End();

		RefPtr<Fence> fence = CommandBufferUtils::ExecuteCommandBufferWithNewFence(commandBuffer);
		fence->WaitUntilSignaled();

		uint8_t* mappedMemory = stagingAlloc->Map<uint8_t>();

		// #TODO_Ivar: Implement support for 3D images.
		const uint32_t perPixelSize = Utility::GetByteSizePerPixelFromFormat(m_desc.format);
		const uint32_t bufferIndex = (x + y * m_desc.width) * perPixelSize;

		Buffer buffer{ stride };
		buffer.Copy(&mappedMemory[bufferIndex], stride);

		stagingAlloc->Unmap();

		GraphicsContext::GetDefaultAllocator()->DestroyBuffer(stagingAlloc);
		return buffer;
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
	}

	void VulkanImage::TransitionToLayout(ImageLayout targetLayout)
	{
		RefPtr<CommandBuffer> commandBuffer = CommandBuffer::Create();
		commandBuffer->Begin();

		RHI::ResourceBarrierInfo barrier{};
		barrier.type = RHI::BarrierType::Image;
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

		RefPtr<Fence> fence = CommandBufferUtils::ExecuteCommandBufferWithNewFence(commandBuffer);
		fence->WaitUntilSignaled();
	}

	void VulkanImage::InitializeWithData(const void* data)
	{
		// #TODO_Ivar: Implement correct size for layer + mip
		const VkDeviceSize bufferSize = m_desc.width * m_desc.height * Utility::GetByteSizePerPixelFromFormat(m_desc.format) * m_desc.layers;

		BufferDesc stagingDesc{};
		stagingDesc.count = 1;
		stagingDesc.elementSize = bufferSize;
		stagingDesc.usage = BufferUsage::TransferSrc;
		stagingDesc.memoryUsage = MemoryUsage::CPUToGPU;
		stagingDesc.debugName = "Staging Alloc";

		Handle<Allocation> stagingAlloc = GraphicsContext::GetDefaultAllocator()->CreateBuffer(stagingDesc);

		auto* stagingData = stagingAlloc->Map<void>();
		memcpy_s(stagingData, bufferSize, data, bufferSize);
		stagingAlloc->Unmap();

		RefPtr<CommandBuffer> commandBuffer = CommandBuffer::Create();

		commandBuffer->Begin();

		{
			RHI::ResourceBarrierInfo barrier{};
			barrier.type = RHI::BarrierType::Image;
			ResourceUtility::InitializeBarrierSrcFromCurrentState(barrier.imageBarrier(), this);

			barrier.imageBarrier().dstStage = RHI::BarrierStage::Copy;
			barrier.imageBarrier().dstAccess = RHI::BarrierAccess::CopyDest;
			barrier.imageBarrier().dstLayout = RHI::ImageLayout::CopyDest;
			barrier.imageBarrier().resource = this;

			commandBuffer->ResourceBarrier({ barrier });
		}

		commandBuffer->CopyBufferToImage(stagingAlloc, this, m_desc.width, m_desc.height, m_desc.depth);

		{
			RHI::ResourceBarrierInfo barrier{};
			barrier.type = RHI::BarrierType::Image;
			ResourceUtility::InitializeBarrierSrcFromCurrentState(barrier.imageBarrier(), this);

			barrier.imageBarrier().dstStage = RHI::BarrierStage::PixelShader | RHI::BarrierStage::ComputeShader;
			barrier.imageBarrier().dstAccess = RHI::BarrierAccess::ShaderRead;
			barrier.imageBarrier().dstLayout = RHI::ImageLayout::ShaderRead;
			barrier.imageBarrier().resource = this;

			commandBuffer->ResourceBarrier({ barrier });
		}

		commandBuffer->End();

		RefPtr<Fence> fence = CommandBufferUtils::ExecuteCommandBufferWithNewFence(commandBuffer);
		fence->WaitUntilSignaled();

		GraphicsContext::GetDefaultAllocator()->DestroyBuffer(stagingAlloc);
	}
}
