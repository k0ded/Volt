#include "dxpch.h"

#include "D3D12RHIModule/Synchronization/D3D12Fence.h"

namespace Volt::RHI
{
	D3D12Fence::D3D12Fence()
	{

	}

	D3D12Fence::~D3D12Fence()
	{

	}

	void D3D12Fence::WaitUntilSignaled() const
	{
		if (m_referencedFence)
		{
			if (m_referencedFence->GetCompletedValue() < m_referencedValue)
			{
				m_referencedFence->SetEventOnCompletion(m_referencedValue, m_referencedWindowsEventHandle);
				::WaitForSingleObject(m_referencedWindowsEventHandle, INFINITE);
			}
		}
	}

	bool D3D12Fence::IsSignaled() const
	{
		if (m_referencedFence)
		{
			return m_referencedFence->GetCompletedValue() >= m_referencedValue;
		}

		// If no fence is referenced, we treat it as signaled.
		return true;
	}

	void* D3D12Fence::GetHandleImpl() const
	{
		return m_referencedFence.Get();
	}

	void D3D12Fence::Reset()
	{
		m_referencedFence = nullptr;
		m_referencedWindowsEventHandle = nullptr;
	}
}
