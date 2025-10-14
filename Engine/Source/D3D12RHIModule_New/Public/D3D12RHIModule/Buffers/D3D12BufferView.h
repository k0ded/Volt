#pragma once

#include "D3D12RHIModule/Common/D3D12Common.h"
#include "D3D12RHIModule/Descriptors/DescriptorCommon.h"

#include <RHIModule/Buffers/BufferView.h>

namespace Volt::RHI
{
	class D3D12BufferView final : public BufferView
	{
	public:
		D3D12BufferView(const BufferViewDesc& desc, RawPtr<StorageBuffer> buffer);
		D3D12BufferView(const BufferViewDesc& desc, RawPtr<UniformBuffer> buffer);
		~D3D12BufferView() override;

		VT_NODISCARD const uint64_t GetDeviceAddress() const override;
		bool IsTexelBufferView() const override;

		VT_NODISCARD VT_INLINE const BufferViewDesc& GetDesc() const override { return m_desc; }
		
		VT_NODISCARD VT_INLINE const D3D12DescriptorPointer& GetSRVDescriptor() const { return m_srvDescriptor; }
		VT_NODISCARD VT_INLINE const D3D12DescriptorPointer& GetUAVDescriptor() const { return m_uavDescriptor; }
		VT_NODISCARD VT_INLINE const D3D12DescriptorPointer& GetCBVDescriptor() const { return m_cbvDescriptor; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void CreateSRV();
		void CreateUAV();
		void CreateCBV();

		D3D12ViewType m_viewType = D3D12ViewType::None;

		D3D12DescriptorPointer m_srvDescriptor;
		D3D12DescriptorPointer m_uavDescriptor;
		D3D12DescriptorPointer m_cbvDescriptor;

		BufferViewDesc m_desc;
		RawPtr<RHIResource> m_resource;
	};
}
