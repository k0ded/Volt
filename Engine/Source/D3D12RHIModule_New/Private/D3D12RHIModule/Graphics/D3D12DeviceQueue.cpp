#include "dxpch.h"

#include "D3D12RHIModule/Graphics/D3D12DeviceQueue.h"
#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"
#include "D3D12RHIModule/Synchronization/D3D12Fence.h"
#include "D3D12RHIModule/Buffers/D3D12CommandBuffer.h"

#include <CoreUtilities/Containers/VectorVariants.h>

namespace Volt::RHI
{
	D3D12DeviceQueue::D3D12DeviceQueue(const DeviceQueueCreateInfo& createInfo)
		: m_queueType(createInfo.queueType)
	{
		CreateCommandQueue(createInfo.graphicsDevice);
		CreateQueueFence(createInfo.graphicsDevice);
	}

	D3D12DeviceQueue::~D3D12DeviceQueue()
	{
		WaitForQueue();

		if (m_windowsFenceEvent)
		{
			::CloseHandle(m_windowsFenceEvent);
		}

		m_fence = nullptr;
		m_commandQueue = nullptr;
	}

	void* D3D12DeviceQueue::GetHandleImpl() const
	{
		return m_commandQueue.Get();
	}

	void D3D12DeviceQueue::CreateCommandQueue(GraphicsDevice* graphicsDevice)
	{
		D3D12GraphicsDevice* d3d12Device = graphicsDevice->As<D3D12GraphicsDevice>();

		D3D12_COMMAND_LIST_TYPE d3d12QueueType = D3D12_COMMAND_LIST_TYPE_NONE;
		switch (m_queueType)
		{
			case QueueType::Graphics: d3d12QueueType = D3D12_COMMAND_LIST_TYPE_DIRECT; break;
			case QueueType::Compute: d3d12QueueType = D3D12_COMMAND_LIST_TYPE_COMPUTE; break;
			case QueueType::TransferCopy: d3d12QueueType = D3D12_COMMAND_LIST_TYPE_COPY; break;
		}

		VT_ENSURE_MSG(d3d12QueueType != D3D12_COMMAND_LIST_TYPE_NONE, "Invalid queue type!");

		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = d3d12QueueType;
		queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.NodeMask = 0;

		VT_D3D12_CHECK(d3d12Device->GetDevice10()->CreateCommandQueue(&queueDesc, VT_D3D12_ID(m_commandQueue)));

		std::wstring name;

		name = L"CommandQueue [";
		switch (m_queueType)
		{
			case QueueType::Graphics:
				name += L"Graphics";
				break;
			case QueueType::Compute:
				name += L"Compute";
				break;
			case QueueType::TransferCopy:
				name += L"TransferCopy";
				break;
			default:
				break;
		}
		name += L"]";

		m_commandQueue->SetName(name.c_str());
	}

	void D3D12DeviceQueue::WaitForQueue()
	{
		if (m_fence->GetCompletedValue() < m_semaphoreValue - 1)
		{
			m_fence->SetEventOnCompletion(m_semaphoreValue - 1, m_windowsFenceEvent);
			::WaitForSingleObject(m_windowsFenceEvent, INFINITE);
		}
	}

	void D3D12DeviceQueue::Execute(const DeviceQueueExecuteInfo& executeInfo)
	{
		InlineVector<ID3D12CommandList*, 64> d3d12CommandLists;
		d3d12CommandLists.reserve(executeInfo.commandBuffers.size());

		for (const auto& cmdList : executeInfo.commandBuffers)
		{
			D3D12CommandBuffer& d3d12CommandBuffer = cmdList->AsRef<D3D12CommandBuffer>();
			d3d12CommandBuffer.m_submissionFence = Fence::Create();

			d3d12CommandLists.emplace_back(d3d12CommandBuffer.GetHandle<ID3D12CommandList*>());
		}

		uint64_t submitSemaphoreValue;
		{
			std::scoped_lock lock{ m_executeMutex };
			submitSemaphoreValue = m_semaphoreValue++;

			m_commandQueue->ExecuteCommandLists(static_cast<uint32_t>(d3d12CommandLists.size()), d3d12CommandLists.data());
			::ResetEvent(m_windowsFenceEvent);
			m_commandQueue->Signal(m_fence.Get(), submitSemaphoreValue);
		}

		for (const auto& fence : executeInfo.signalFences)
		{

			D3D12Fence& d3d12Fence = fence->AsRef<D3D12Fence>();
			d3d12Fence.m_referencedValue = submitSemaphoreValue;
			d3d12Fence.m_referencedFence = m_fence;
			d3d12Fence.m_referencedWindowsEventHandle = m_windowsFenceEvent;
		}

		if (executeInfo.executionFence)
		{
			D3D12Fence& d3d12Fence = executeInfo.executionFence->AsRef<D3D12Fence>();
			d3d12Fence.m_referencedValue = submitSemaphoreValue;
			d3d12Fence.m_referencedFence = m_fence;
			d3d12Fence.m_referencedWindowsEventHandle = m_windowsFenceEvent;
		}

		for (const auto& cmdList : executeInfo.commandBuffers)
		{
			D3D12CommandBuffer& d3d12CommandBuffer = cmdList->AsRef<D3D12CommandBuffer>();
			D3D12Fence& d3d12Fence = d3d12CommandBuffer.m_submissionFence->AsRef<D3D12Fence>();
			d3d12Fence.m_referencedValue = submitSemaphoreValue;
			d3d12Fence.m_referencedFence = m_fence;
			d3d12Fence.m_referencedWindowsEventHandle = m_windowsFenceEvent;
		}
	}

	void D3D12DeviceQueue::CreateQueueFence(GraphicsDevice* graphicsDevice)
	{
		ID3D12Device10* d3d12Device = graphicsDevice->AsRef<D3D12GraphicsDevice>().GetDevice10();
		VT_D3D12_CHECK(d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, VT_D3D12_ID(m_fence)));
		m_windowsFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	}
}
