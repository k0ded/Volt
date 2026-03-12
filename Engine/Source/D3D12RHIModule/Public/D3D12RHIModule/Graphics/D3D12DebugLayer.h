#pragma once

#include "D3D12RHIModule/Common/ComPtr.h"

struct ID3D12Debug;
struct ID3D12InfoQueue1;

namespace Volt::RHI
{
	class D3D12DebugLayer
	{
	public:
		D3D12DebugLayer();
		~D3D12DebugLayer();

		void InitializeAPIValidation(RawPtr<GraphicsDevice> graphicsDevice);

		VT_NODISCARD VT_INLINE bool IsSupported() const { return m_isSupported; }

	private:
		void InitializeDebugLayer();

		ComPtr<ID3D12Debug> m_debugInterface;
		ComPtr<ID3D12InfoQueue1> m_infoQueue;

		unsigned long m_debugCallbackId = 0;

		bool m_isSupported = false;
	};
}
