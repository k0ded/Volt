#pragma once

#include "VulkanRHIModule/Core.h"
#include <RHIModule/Buffers/BufferView.h>

#include <vulkan/vulkan.h>

struct VkBufferView_T;

namespace Volt::RHI
{
	class VulkanBufferView final : public BufferView
	{
	public:
		struct DescriptorDescription
		{
			VkDescriptorGetInfoEXT vkDescriptorInfo;
			VkDescriptorAddressInfoEXT addressInfo;
			uint64_t descriptorSize;
		};

		VulkanBufferView(const BufferViewDesc& desc, RawPtr<StorageBuffer> buffer);
		VulkanBufferView(const BufferViewDesc& desc, RawPtr<UniformBuffer> buffer);
		~VulkanBufferView() override;

		VT_NODISCARD const uint64_t GetDeviceAddress() const override;

		RawPtr<RHIResource> GetResource() const { return m_resource; }
		bool IsTexelBufferView() const override;

		VT_NODISCARD VT_INLINE const BufferViewDesc& GetDesc() const override { return m_desc; }
		VT_NODISCARD VT_INLINE const DescriptorDescription& GetSRVDescriptor() const { return m_srvDescriptor; }
		VT_NODISCARD VT_INLINE const DescriptorDescription& GetUAVDescriptor() const { return m_uavDescriptor; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void CreateView();

		BufferViewDesc m_desc;
		RawPtr<RHIResource> m_resource;

		DescriptorDescription m_srvDescriptor;
		DescriptorDescription m_uavDescriptor;
	};
}
