#include "dxpch.h"

#include "D3D12RHIModule/Images/D3D12Image.h"
#include "D3D12RHIModule/Graphics/D3D12Swapchain.h"

#include <RHIModule/Images/ImageUtility.h>
#include <RHIModule/Utility/ResourceUtility.h>
#include <RHIModule/Buffers/CommandBufferUtility.h>
#include <RHIModule/Memory/Allocation.h>

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt::RHI
{
	D3D12Image::D3D12Image(const ImageDesc& desc, const void* data, RefPtr<GPUAllocator> allocator)
		: m_desc(desc), m_allocator(allocator)
	{
		if (!allocator)
		{
			m_allocator = GraphicsContext::GetDefaultAllocator();
		}

		Invalidate(desc.width, desc.height, desc.depth, data);
		SetName(desc.debugName);
	}

	D3D12Image::D3D12Image(const SwapchainImageDesc& desc)
		: m_isSwapchainImage(true)
	{
		GraphicsContext::GetResourceStateTracker()->AddResource(this, BarrierStage::None, BarrierAccess::None, ImageLayout::Undefined);

		InvalidateSwapchainImage(desc);
		m_desc.debugName = std::format("Swapchain Image {}", desc.imageIndex);
		SetName(m_desc.debugName);
	}

	D3D12Image::~D3D12Image()
	{
		GraphicsContext::GetResourceStateTracker()->RemoveResource(this);
		Release();
	}

	void D3D12Image::Invalidate(const uint32_t width, const uint32_t height, const uint32_t depth, const void* data)
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

	void D3D12Image::Release()
	{
		if (!m_allocation)
		{
			return;
		}

		m_allocator->DestroyImage(m_allocation);
		m_allocation = nullptr;
	}

	void D3D12Image::GenerateMips()
	{
		VT_ENSURE(false);
	}

	RefPtr<ImageView> D3D12Image::GetView(const ImageViewDesc& desc)
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

		return ImageView::Create(tempDesc, this);
	}

	const uint32_t D3D12Image::CalculateMipCount() const
	{
		return Utility::CalculateMipCount(m_desc.width, m_desc.height);
	}

	void D3D12Image::SetName(const std::string& name)
	{
		m_desc.debugName = name;

		std::wstring wname(name.begin(), name.end());
		if (m_isSwapchainImage)
		{
			m_swapchainImageData.image->SetName(wname.c_str());
		}
		else
		{
			m_allocation->GetResourceHandle<ID3D12Resource*>()->SetName(wname.c_str());
		}
	}

	std::string_view D3D12Image::GetName() const
	{
		return m_desc.debugName;
	}

	const uint64_t D3D12Image::GetDeviceAddress() const
	{
		return m_allocation->GetDeviceAddress();
	}

	const uint64_t D3D12Image::GetByteSize() const
	{
		return m_allocation->GetSize();
	}

	void* D3D12Image::GetHandleImpl() const
	{
		if (m_isSwapchainImage)
		{
			return m_swapchainImageData.image;
		}
		else
		{
			return m_allocation->GetResourceHandle<ID3D12Resource*>();
		}
	}

	Buffer D3D12Image::ReadPixelInternal(const uint32_t x, const uint32_t y, const uint32_t z, const size_t stride)
	{
		VT_ENSURE(false);
		return {};
	}

	void D3D12Image::InvalidateSwapchainImage(const SwapchainImageDesc& specification)
	{
		const auto& d3d12Swapchain = specification.swapchain->AsRef<D3D12Swapchain>();

		m_imageAspect = ImageAspect::Color;
		m_desc.width = d3d12Swapchain.GetWidth();
		m_desc.height = d3d12Swapchain.GetHeight();
		m_desc.format = d3d12Swapchain.GetFormat();
		m_desc.usage = ImageUsage::Attachment;

		m_swapchainImageData.image = d3d12Swapchain.GetImageAtIndex(specification.imageIndex).Get();
	}

	void D3D12Image::TransitionToLayout(ImageLayout targetLayout)
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

	void D3D12Image::InitializeWithData(const void* data)
	{
		// #TODO_Ivar: Implement correct size for layer + mip
		const uint64_t bufferSize = m_desc.width * m_desc.height * Utility::GetByteSizePerPixelFromFormat(m_desc.format) * m_desc.layers;

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
