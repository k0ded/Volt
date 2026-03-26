#include "dxpch.h"

#include "D3D12RHIModule/Buffers/D3D12UniformBuffer.h"

#include <RHIModule/Memory/MemoryUtility.h>
#include <RHIModule/Memory/Allocation.h>
#include <RHIModule/RHIModule.h>
#include <RHIModule/RHICapabilities.h>

#include <CoreUtilities/StringUtility.h>

namespace Volt::RHI
{
	D3D12UniformBuffer::D3D12UniformBuffer(const UniformBufferDesc& desc, const void* initialData)
		: m_desc(desc)
	{
		GraphicsContext::GetResourceStateTracker()->AddResource(this, BarrierStage::None, BarrierAccess::None);

		BufferDesc bufferDesc{};
		bufferDesc.count = 1;
		bufferDesc.elementSize = Utility::Align(desc.size, g_rhiCapabilities.minUniformBufferAlignment);
		bufferDesc.usage = BufferUsage::UniformBuffer | BufferUsage::DeviceAddress;
		bufferDesc.memoryUsage = MemoryUsage::CPUToGPU;
		bufferDesc.debugName = desc.debugName;

		m_allocation = GraphicsContext::GetDefaultAllocator()->CreateBuffer(bufferDesc);

		if (initialData)
		{
			SetData(initialData, desc.size);
		}

		SetName(desc.debugName);
	}

	D3D12UniformBuffer::~D3D12UniformBuffer()
	{
		GraphicsContext::GetResourceStateTracker()->RemoveResource(this);

		if (m_allocation == nullptr)
		{
			return;
		}

		RHIModule::GetInstance().DestroyResource([allocation = m_allocation]()
		{
			GraphicsContext::GetDefaultAllocator()->DestroyBuffer(allocation);
		});

		m_allocation = nullptr;
	}

	IntRef<BufferView> D3D12UniformBuffer::GetView(const BufferViewDesc& desc)
	{
		return BufferView::Create(desc, this);
	}

	const uint32_t D3D12UniformBuffer::GetSize() const
	{
		return m_desc.size;
	}

	void D3D12UniformBuffer::SetData(const void* data, const uint32_t size)
	{
		void* bufferData = m_allocation->Map<void>();
		memcpy_s(bufferData, m_desc.size, data, size);
		m_allocation->Unmap();
	}

	void D3D12UniformBuffer::Unmap()
	{
		m_allocation->Unmap();
	}

	void D3D12UniformBuffer::SetName(const std::string& name)
	{
		m_desc.debugName = name;
		if (!m_allocation)
		{
			return;
		}

		std::wstring str = ::Utility::ToWString(name);
		m_allocation->GetResourceHandle<ID3D12Resource*>()->SetName(str.c_str());
	}

	StringView D3D12UniformBuffer::GetName() const
	{
		return m_desc.debugName;
	}

	const uint64_t D3D12UniformBuffer::GetDeviceAddress() const
	{
		return m_allocation->GetDeviceAddress();
	}

	const uint64_t D3D12UniformBuffer::GetByteSize() const
	{
		return m_allocation->GetSize();
	}

	void* D3D12UniformBuffer::MapInternal(const uint32_t index)
	{
		uint8_t* bytePtr = m_allocation->Map<uint8_t>();
		return bytePtr;
	}

	void* D3D12UniformBuffer::GetHandleImpl() const
	{
		return m_allocation->GetResourceHandle<ID3D12Resource*>();
	}
}
