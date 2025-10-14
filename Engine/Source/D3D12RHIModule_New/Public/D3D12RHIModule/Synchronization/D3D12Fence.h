#pragma once

#include "D3D12RHIModule/Common/ComPtr.h"

#include <RHIModule/Synchronization/Fence.h>

namespace Volt::RHI
{
	class D3D12Fence : public Fence
	{
	public:
		D3D12Fence();
		~D3D12Fence() override;

		void WaitUntilSignaled() const override;
		bool IsSignaled() const override;
		void Reset() override;

	protected:
		void* GetHandleImpl() const override;

	private:
		friend class D3D12DeviceQueue;

		ComPtr<ID3D12Fence> m_referencedFence;
		void* m_referencedWindowsEventHandle = nullptr;

		uint64_t m_referencedValue = 0;
	};
}
