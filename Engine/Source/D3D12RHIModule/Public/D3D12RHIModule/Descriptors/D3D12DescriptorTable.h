#pragma once

#include "D3D12RHIModule/Common/ComPtr.h"
#include "D3D12RHIModule/Descriptors/DescriptorCommon.h"
#include "D3D12RHIModule/Common/D3D12Common.h"

#include <RHIModule/Descriptors/DescriptorTable.h>
#include <RHIModule/Shader/Shader.h>

#include <CoreUtilities/Containers/Map.h>

namespace Volt::RHI
{
	class D3D12DescriptorHeap;
	class D3D12DescriptorTable : public DescriptorTable
	{
	public:
		D3D12DescriptorTable(const DescriptorTableCreateInfo& createInfo);
		~D3D12DescriptorTable() override;

		void SetImageView(RawPtr<ImageView> imageView, uint32_t set, uint32_t binding, uint32_t arrayIndex = 0) override;
		void SetBufferView(RawPtr<BufferView> bufferView, uint32_t set, uint32_t binding, uint32_t arrayIndex = 0) override;
		void SetSamplerState(RawPtr<SamplerState> samplerState, uint32_t set, uint32_t binding, uint32_t arrayIndex /* = 0 */) override;

		void SetImageView(std::string_view name, RawPtr<ImageView> view, uint32_t arrayIndex = 0) override;
		void SetBufferView(std::string_view name, RawPtr<BufferView> view, uint32_t arrayIndex = 0) override;
		void SetSamplerState(std::string_view name, RawPtr<SamplerState> samplerState, uint32_t arrayIndex = 0) override;

		void PrepareForRender() override;
		void Bind(CommandBuffer& commandBuffer) override;
		void SetRootParameters(CommandBuffer& commandBuffer);

	protected:
		void* GetHandleImpl() const override;

	private:
		void SetImageView(RawPtr<ImageView> imageView, uint32_t set, uint32_t binding, ShaderRegisterType registerType);
		void SetBufferView(RawPtr<BufferView> bufferView, uint32_t set, uint32_t binding, ShaderRegisterType registerType);
		void SetSamplerState(RawPtr<SamplerState> samplerState, uint32_t set, uint32_t binding, ShaderRegisterType registerType);

		void Invalidate();
		void Release();

		void CreateDescriptorHeaps(uint32_t mainDescriptorCount, uint32_t samplerDescriptorCount);
		void AllocateDescriptors();

		RawPtr<Shader> m_shader;
		bool m_isDirty = false;
		bool m_isComputeTable = false;
		uint32_t m_descriptorTableRootParamStartIndex = 0;

		Map<uint32_t, Map<uint32_t, Map<ShaderRegisterType, AllocatedDescriptorInfo>>> m_allocatedDescriptorPointers;
		Vector<DescriptorCopyInfo> m_activeDescriptorCopies;
		Vector<DescriptorCopyInfo> m_activeSamplerDescriptorCopies;

		Scope<D3D12DescriptorHeap> m_mainHeap;
		Scope<D3D12DescriptorHeap> m_samplerHeap;
	};
}
