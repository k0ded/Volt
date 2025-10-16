#pragma once

#include "D3D12RHIModule/Descriptors/DescriptorCommon.h"

#include <RHIModule/Images/SamplerState.h>

namespace Volt::RHI
{
	class D3D12SamplerState : public SamplerState
	{
	public:
		D3D12SamplerState(const SamplerStateDesc& createInfo);
		~D3D12SamplerState() override;

		VT_NODISCARD VT_INLINE const D3D12DescriptorPointer& GetDescriptor() const { return m_descriptor; }

	protected:
		void* GetHandleImpl() const override;

	private:
		D3D12DescriptorPointer m_descriptor;
	};
}
