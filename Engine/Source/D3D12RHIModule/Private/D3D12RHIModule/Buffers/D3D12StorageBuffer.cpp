#include "dxpch.h"

#include "D3D12RHIModule/Buffers/D3D12StorageBuffer.h"

#include <RHIModule/Buffers/CommandBuffer.h>

#include <RHIModule/Memory/Allocation.h>
#include <RHIModule/Buffers/CommandBufferUtility.h>
#include <RHIModule/RHIModule.h>

#include <CoreUtilities/EnumUtils.h>

namespace Volt::RHI
{
	D3D12StorageBuffer::D3D12StorageBuffer(const BufferDesc& desc, IntRef<GPUAllocator> allocator)
		: m_allocator(allocator), m_desc(desc)
	{
		GraphicsContext::GetResourceStateTracker()->AddResource(this, BarrierStage::None, BarrierAccess::None);

		if (!m_allocator)
		{
			m_allocator = GraphicsContext::GetDefaultAllocator();
		}

		// Make sure that the desc contains either storage buffer or texel buffer.
		if (!EnumValueContainsFlag(m_desc.usage, BufferUsage::StorageBuffer) && !EnumValueContainsFlag(m_desc.usage, BufferUsage::TexelBuffer))
		{
			m_desc.usage |= BufferUsage::StorageBuffer;
		}

		// Make sure buffer always contains transfer source and transfer dest
		m_desc.usage |= BufferUsage::TransferSrc | BufferUsage::TransferDst | BufferUsage::DeviceAddress;

		Invalidate(desc.elementSize * desc.count);
		SetName(desc.debugName);
	}

	D3D12StorageBuffer::~D3D12StorageBuffer()
	{
		GraphicsContext::GetResourceStateTracker()->RemoveResource(this);
		Release();
	}

	void D3D12StorageBuffer::Resize(const uint64_t byteSize)
	{
		if (byteSize <= m_desc.elementSize * m_desc.count)
		{
			return;
		}

		m_desc.count = static_cast<uint32_t>(byteSize / m_desc.elementSize);

		Invalidate(byteSize);
		SetName(m_desc.debugName);
	}

	void D3D12StorageBuffer::ResizeWithCount(const uint32_t count)
	{
		auto oldAllocation = m_allocation;
		auto oldSize = m_byteSize;

		m_desc.count = count;
		const uint64_t newSize = m_desc.count * m_desc.elementSize;

		// We don't need to do a full recreate if the current buffer can hold the requested count.
		if (newSize <= m_allocation->GetSize())
		{
			return;
		}

		Release();

		m_byteSize = newSize;

		m_allocation = m_allocator->CreateBuffer(m_desc);

		SetName(m_desc.debugName);

		// Copy old data to new buffer
		IntRef<CommandBuffer> commandBuffer = CommandBuffer::Create();

		commandBuffer->Begin();

		ResourceBarrierInfo barrierInfo = ResourceBarrierInfo::InitializeAsGlobalBarrier();
		barrierInfo.globalBarrier().srcStage = BarrierStage::All;
		barrierInfo.globalBarrier().dstStage = BarrierStage::Copy;
		barrierInfo.globalBarrier().srcAccess = BarrierAccess::None;
		barrierInfo.globalBarrier().dstAccess = BarrierAccess::CopyDest | BarrierAccess::CopySource;

		commandBuffer->ResourceBarrier({ barrierInfo });
		commandBuffer->CopyBufferRegion(oldAllocation, 0, m_allocation, 0, oldSize);

		barrierInfo.globalBarrier().srcStage = BarrierStage::Copy;
		barrierInfo.globalBarrier().dstStage = BarrierStage::All;
		barrierInfo.globalBarrier().srcAccess = BarrierAccess::CopyDest | BarrierAccess::CopySource;
		barrierInfo.globalBarrier().dstAccess = BarrierAccess::None;

		commandBuffer->ResourceBarrier({ barrierInfo });

		commandBuffer->End();
		CommandBufferUtils::ExecuteCommandBufferWithNewFence(commandBuffer);
	}

	const size_t D3D12StorageBuffer::GetByteSize() const
	{
		return m_allocation->GetSize();
	}

	const size_t D3D12StorageBuffer::GetElementSize() const
	{
		return m_desc.elementSize;
	}

	const uint32_t D3D12StorageBuffer::GetCount() const
	{
		return m_desc.count;
	}

	Handle<Allocation> D3D12StorageBuffer::GetAllocation() const
	{
		return m_allocation;
	}

	void D3D12StorageBuffer::Unmap()
	{
		m_allocation->Unmap();
	}

	void D3D12StorageBuffer::SetData(const void* data, const size_t size)
	{
		BufferDesc stagingDesc{};
		stagingDesc.count = 1;
		stagingDesc.elementSize = size;
		stagingDesc.usage = BufferUsage::TransferSrc;
		stagingDesc.memoryUsage = MemoryUsage::CPUToGPU;
		stagingDesc.debugName = "Staging Alloc";

		Handle<Allocation> stagingAllocation = GraphicsContext::GetDefaultAllocator()->CreateBuffer(stagingDesc);

		void* mappedPtr = stagingAllocation->Map<void>();
		memcpy_s(mappedPtr, size, data, size);
		stagingAllocation->Unmap();

		IntRef<CommandBuffer> cmdBuffer = CommandBuffer::Create();
		cmdBuffer->Begin();
		cmdBuffer->BeginMarker(std::format("Updating data in {}", m_desc.debugName), { 1.f, 1.f, 1.f, 1.f });

		ResourceBarrierInfo barrier = ResourceBarrierInfo::InitializeAsBufferBarrier();
		barrier.bufferBarrier().srcStage = BarrierStage::ComputeShader | BarrierStage::VertexShader | BarrierStage::PixelShader;
		barrier.bufferBarrier().srcAccess = BarrierAccess::None;
		barrier.bufferBarrier().dstStage = BarrierStage::Copy;
		barrier.bufferBarrier().dstAccess = BarrierAccess::CopyDest;
		barrier.bufferBarrier().offset = 0;
		barrier.bufferBarrier().size = m_allocation->GetSize();
		barrier.bufferBarrier().resource = RawPtr<D3D12StorageBuffer>(this);

		cmdBuffer->ResourceBarrier({ barrier });

		cmdBuffer->CopyBufferRegion(stagingAllocation, 0, m_allocation, 0, size);

		barrier.bufferBarrier().srcStage = BarrierStage::Copy;
		barrier.bufferBarrier().srcAccess = BarrierAccess::CopyDest;
		barrier.bufferBarrier().dstStage = BarrierStage::ComputeShader | BarrierStage::VertexShader | BarrierStage::PixelShader;
		barrier.bufferBarrier().dstAccess = BarrierAccess::None;

		cmdBuffer->ResourceBarrier({ barrier });

		cmdBuffer->EndMarker();
		cmdBuffer->End();
		CommandBufferUtils::ExecuteCommandBufferWithNewFence(cmdBuffer);

		GraphicsContext::GetDefaultAllocator()->DestroyBuffer(stagingAllocation);
	}

	void D3D12StorageBuffer::SetData(IntRef<CommandBuffer> commandBuffer, const void* data, const size_t size)
	{
		BufferDesc stagingDesc{};
		stagingDesc.count = 1;
		stagingDesc.elementSize = size;
		stagingDesc.usage = BufferUsage::TransferSrc;
		stagingDesc.memoryUsage = MemoryUsage::CPUToGPU;
		stagingDesc.debugName = "Staging Alloc";

		Handle<Allocation> stagingAllocation = GraphicsContext::GetDefaultAllocator()->CreateBuffer(stagingDesc);

		void* mappedPtr = stagingAllocation->Map<void>();
		memcpy_s(mappedPtr, m_byteSize, data, size);
		stagingAllocation->Unmap();

		ResourceBarrierInfo barrier = ResourceBarrierInfo::InitializeAsBufferBarrier();
		barrier.bufferBarrier().srcStage = BarrierStage::All;
		barrier.bufferBarrier().srcAccess = BarrierAccess::None;
		barrier.bufferBarrier().dstStage = BarrierStage::Copy;
		barrier.bufferBarrier().dstAccess = BarrierAccess::CopyDest;
		barrier.bufferBarrier().offset = 0;
		barrier.bufferBarrier().size = size;
		barrier.bufferBarrier().resource = RawPtr<D3D12StorageBuffer>(this);

		commandBuffer->ResourceBarrier({ barrier });

		commandBuffer->CopyBufferRegion(stagingAllocation, 0, m_allocation, 0, size);

		barrier.bufferBarrier().srcStage = BarrierStage::Copy;
		barrier.bufferBarrier().srcAccess = BarrierAccess::CopyDest;
		barrier.bufferBarrier().dstStage = BarrierStage::All;
		barrier.bufferBarrier().dstAccess = BarrierAccess::None;

		commandBuffer->ResourceBarrier({ barrier });

		GraphicsContext::GetDefaultAllocator()->DestroyBuffer(stagingAllocation);
	}

	IntRef<BufferView> D3D12StorageBuffer::GetView(const BufferViewDesc& desc)
	{
		IntRef<BufferView> bufferView = BufferView::Create(desc, this);
		return bufferView;
	}

	void D3D12StorageBuffer::SetName(const std::string& name)
	{
		m_desc.debugName = name;

		std::wstring wname(name.begin(), name.end());
		m_allocation->GetResourceHandle<ID3D12Resource*>()->SetName(wname.c_str());
	}

	StringView D3D12StorageBuffer::GetName() const
	{
		return m_desc.debugName;
	}

	const uint64_t D3D12StorageBuffer::GetDeviceAddress() const
	{
		return m_allocation->GetDeviceAddress();
	}

	void* D3D12StorageBuffer::GetHandleImpl() const
	{
		return m_allocation->GetResourceHandle<ID3D12Resource*>();
	}

	void* D3D12StorageBuffer::MapInternal()
	{
		return m_allocation->Map<void>();
	}

	void D3D12StorageBuffer::Invalidate(const uint64_t byteSize)
	{
		Release();
		m_byteSize = byteSize;
		m_allocation = m_allocator->CreateBuffer(m_desc);
	}

	void D3D12StorageBuffer::Release()
	{
		if (!m_allocation)
		{
			return;
		}

		RHIModule::GetInstance().DestroyResource([allocator = m_allocator, allocation = m_allocation]()
		{
			allocator->DestroyBuffer(allocation);
		});
		m_allocation = nullptr;
	}

	const BufferDesc& D3D12StorageBuffer::GetDesc() const
	{
		return m_desc;
	}
}
