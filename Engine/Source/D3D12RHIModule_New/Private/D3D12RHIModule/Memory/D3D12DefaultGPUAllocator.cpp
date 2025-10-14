#include "dxpch.h"
#include "D3D12RHIModule/Memory/D3D12DefaultGPUAllocator.h"

#include "D3D12RHIModule/Common/D3D12MemAlloc.h"
#include "D3D12RHIModule/Common/D3D12Helpers.h"
#include "D3D12RHIModule/Memory/D3D12Allocation.h"
#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"

#include <RHIModule/Utility/HashUtility.h>
#include <RHIModule/Images/ImageUtility.h>

#include <CoreUtilities/Profiling/Profiling.h>

#include <CoreUtilities/EnumUtils.h>

namespace Volt::RHI
{
	D3D12DefaultGPUAllocator::D3D12DefaultGPUAllocator()
	{
		auto device = GraphicsContext::GetDevice()->As<D3D12GraphicsDevice>()->GetDevice10();
		auto adapter = GraphicsContext::GetPhysicalDevice()->GetHandle<IDXGIAdapter4*>();

		D3D12MA::ALLOCATOR_DESC desc{};
		desc.pAdapter = adapter;
		desc.pDevice = device;
		desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;

		VT_D3D12_CHECK(D3D12MA::CreateAllocator(&desc, &m_allocator));

		m_bufferAllocationArena.AllocateArena(16384);
		m_imageAllocationArena.AllocateArena(16384);
	}

	D3D12DefaultGPUAllocator::~D3D12DefaultGPUAllocator()
	{
		VT_D3D12_DELETE(m_allocator);
	}

	Handle<Allocation> D3D12DefaultGPUAllocator::CreateBuffer(const BufferDesc& desc)
	{
		VT_PROFILE_FUNCTION();

		const uint64_t byteSize = desc.count * desc.elementSize;

		const size_t hash = Utility::GetHashFromBufferSpec(byteSize, desc.usage, desc.memoryUsage);
		if (auto buffer = m_allocationCache.TryGetBufferAllocationFromHash(hash))
		{
			return buffer;
		}

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = 0;
		resourceDesc.Width = byteSize;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc = { 1, 0 };
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		if (EnumValueContainsFlag(desc.memoryUsage, MemoryUsage::GPU))
		{
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		D3D12MA::ALLOCATION_DESC allocDesc{};
		allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		allocDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_NONE;
		allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;

		if (EnumValueContainsFlag(desc.memoryUsage, MemoryUsage::CPUToGPU))
		{
			allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
		}
		else if (EnumValueContainsFlag(desc.memoryUsage, MemoryUsage::GPUToCPU))
		{
			allocDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
		}

		if (EnumValueContainsFlag(desc.memoryUsage, MemoryUsage::Dedicated))
		{
			allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
		}

		Handle<D3D12BufferAllocation> allocation = m_bufferAllocationArena.Allocate(hash, desc.debugName);
		VT_D3D12_CHECK(m_allocator->CreateResource(&allocDesc, &resourceDesc, Utility::GetResourceStateFromUsage(desc.usage), nullptr, &allocation->m_allocation, IID_PPV_ARGS(&allocation->m_resource)));

		allocation->m_size = byteSize;

		return allocation;
	}

	Handle<Allocation> D3D12DefaultGPUAllocator::CreateImage(const ImageDesc& imageSpecification, MemoryUsage memoryUsage)
	{
		VT_PROFILE_FUNCTION();

		const size_t hash = Utility::GetHashFromImageSpec(imageSpecification, memoryUsage);
		if (auto buffer = m_allocationCache.TryGetImageAllocationFromHash(hash))
		{
			return buffer;
		}

		D3D12_RESOURCE_DESC1 resourceDesc = Utility::GetD3D12ResourceDesc(imageSpecification);

		D3D12MA::ALLOCATION_DESC allocDesc{};
		allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		allocDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_NONE;
		allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;

		if ((memoryUsage & MemoryUsage::CPUToGPU) != MemoryUsage::None)
		{
			allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
		}
		else if ((memoryUsage & MemoryUsage::GPUToCPU) != MemoryUsage::None)
		{
			allocDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
		}

		if ((memoryUsage & MemoryUsage::Dedicated) != MemoryUsage::None)
		{
			allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
		}

		Handle<D3D12ImageAllocation> allocation = m_imageAllocationArena.Allocate(hash, imageSpecification.debugName);
		VT_D3D12_CHECK(m_allocator->CreateResource2(&allocDesc, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, &allocation->m_allocation, IID_PPV_ARGS(&allocation->m_resource)));

		allocation->m_size = allocation->m_allocation->GetSize();

		return allocation;
	}

	void D3D12DefaultGPUAllocator::DestroyBuffer(Handle<Allocation> allocation)
	{
		m_allocationCache.QueueBufferAllocationForRemoval(allocation);
	}

	void D3D12DefaultGPUAllocator::DestroyImage(Handle<Allocation> allocation)
	{
		m_allocationCache.QueueImageAllocationForRemoval(allocation);
	}

	Vector<Handle<Allocation>> D3D12DefaultGPUAllocator::GetActiveBufferAllocations() const
	{
		return Vector<Handle<Allocation>>();
	}

	Vector<Handle<Allocation>> D3D12DefaultGPUAllocator::GetActiveImageAllocations() const
	{
		return Vector<Handle<Allocation>>();
	}

	void D3D12DefaultGPUAllocator::Update()
	{
		const auto allocationsToRemove = m_allocationCache.UpdateAndGetAllocationsToDestroy();

		for (const auto& alloc : allocationsToRemove.bufferAllocations)
		{
			DestroyBufferInternal(alloc);
		}

		for (const auto& alloc : allocationsToRemove.imageAllocations)
		{
			DestroyImageInternal(alloc);
		}
	}

	void* D3D12DefaultGPUAllocator::GetHandleImpl() const
	{
		return m_allocator;
	}

	void D3D12DefaultGPUAllocator::DestroyBufferInternal(Handle<Allocation> allocation)
	{
		auto bufferAlloc = allocation.As<D3D12BufferAllocation>();
		bufferAlloc->m_allocation->Release();
		bufferAlloc->m_resource->Release();

		m_bufferAllocationArena.Free(bufferAlloc.GetRaw());
	}

	void D3D12DefaultGPUAllocator::DestroyImageInternal(Handle<Allocation> allocation)
	{
		auto imageAlloc = allocation.As<D3D12ImageAllocation>();
		imageAlloc->m_allocation->Release();
		imageAlloc->m_resource->Release();

		m_imageAllocationArena.Free(imageAlloc.GetRaw());
	}
}
