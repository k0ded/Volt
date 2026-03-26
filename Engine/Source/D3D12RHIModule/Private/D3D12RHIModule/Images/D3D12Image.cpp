#include "dxpch.h"

#include "D3D12RHIModule/Images/D3D12Image.h"
#include "D3D12RHIModule/Graphics/D3D12Swapchain.h"
#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"

#include <RHIModule/Images/ImageUtility.h>
#include <RHIModule/Utility/ResourceUtility.h>
#include <RHIModule/Buffers/CommandBufferUtility.h>
#include <RHIModule/Memory/Allocation.h>

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt::RHI
{
	D3D12Image::D3D12Image(const ImageDesc& desc, const void* data, IntRef<GPUAllocator> allocator)
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
		BarrierAccess barrierAccess = BarrierAccess::None;

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

				barrierAccess = BarrierAccess::RenderTarget;
			}

			case ImageUsage::Texture:
			{
				targetLayout = ImageLayout::ShaderRead;
				barrierAccess = BarrierAccess::ShaderRead;
				break;
			}

			case ImageUsage::Storage:
			{
				targetLayout = ImageLayout::ShaderWrite;
				barrierAccess = BarrierAccess::ShaderWrite;
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

	IntRef<ImageView> D3D12Image::GetView(const ImageViewDesc& desc)
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

	StringView D3D12Image::GetName() const
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
		ID3D12Device10* d3d12Device = GraphicsContext::GetDevice()->As<D3D12GraphicsDevice>()->GetDevice10();

		ID3D12Resource* imageResource = m_allocation->GetResourceHandle<ID3D12Resource*>();

		D3D12_RESOURCE_DESC desc = imageResource->GetDesc();

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
		UINT numRows;
		UINT64 rowSizeInBytes;
		UINT64 totalBytes;
		d3d12Device->GetCopyableFootprints(
			&desc,
			0, 1, 0,
			&footprint,
			&numRows,
			&rowSizeInBytes,
			&totalBytes);

		BufferDesc stagingDesc{};
		stagingDesc.count = 1;
		stagingDesc.elementSize = totalBytes;
		stagingDesc.usage = BufferUsage::TransferDst;
		stagingDesc.memoryUsage = MemoryUsage::GPUToCPU;
		stagingDesc.debugName = "Staging Alloc";

		Handle<Allocation> stagingAlloc = GraphicsContext::GetDefaultAllocator()->CreateBuffer(stagingDesc);

		IntRef<CommandBuffer> commandBuffer = CommandBuffer::Create();
		commandBuffer->Begin();

		RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsImageBarrier();
		ResourceUtility::InitializeBarrierSrcFromCurrentState(barrier.imageBarrier(), this);

		barrier.imageBarrier().dstStage = RHI::BarrierStage::Copy;
		barrier.imageBarrier().dstAccess = RHI::BarrierAccess::CopySource;
		barrier.imageBarrier().dstLayout = RHI::ImageLayout::CopySource;
		barrier.imageBarrier().resource = this;
			
		commandBuffer->ResourceBarrier({ barrier });

		D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
		dstLocation.pResource = stagingAlloc->GetResourceHandle<ID3D12Resource*>();
		dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		dstLocation.PlacedFootprint = footprint;

		D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
		srcLocation.pResource = imageResource;
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		srcLocation.SubresourceIndex = 0;

		commandBuffer->GetHandle<ID3D12GraphicsCommandList7*>()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

		std::swap(barrier.imageBarrier().srcStage, barrier.imageBarrier().dstStage);
		std::swap(barrier.imageBarrier().srcLayout, barrier.imageBarrier().dstLayout);
		std::swap(barrier.imageBarrier().srcAccess, barrier.imageBarrier().dstAccess);

		commandBuffer->ResourceBarrier({ barrier });
		commandBuffer->End();

		IntRef<Fence> fence = CommandBufferUtils::ExecuteCommandBufferWithNewFence(commandBuffer);
		fence->WaitUntilSignaled();

		uint8_t* mappedMemory = stagingAlloc->Map<uint8_t>();
	
		const uint32_t perPixelSize = Utility::GetByteSizePerPixelFromFormat(m_desc.format);
		const uint32_t srcBufferIndex = x * perPixelSize + y * footprint.Footprint.RowPitch;

		Buffer buffer{ stride };
		buffer.Copy(&mappedMemory[srcBufferIndex], stride);

		stagingAlloc->Unmap();
		GraphicsContext::GetDefaultAllocator()->DestroyBuffer(stagingAlloc);

		return buffer;
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

		commandBuffer->CopyBufferToImage(stagingAlloc, this, m_desc.width, m_desc.height, m_desc.depth);

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

		GraphicsContext::GetDefaultAllocator()->DestroyBuffer(stagingAlloc);
	}
}
