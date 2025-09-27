#pragma once

#include "VulkanRHIModule/Core.h"
#include <RHIModule/Buffers/BufferView.h>

struct VkBufferView_T;

namespace Volt::RHI
{
	class VulkanBufferView : public BufferView
	{
	public:
		VulkanBufferView(const BufferViewDesc& desc, RawPtr<StorageBuffer> buffer);
		VulkanBufferView(const BufferViewDesc& desc, RawPtr<UniformBuffer> buffer);
		~VulkanBufferView() override;

		VT_NODISCARD const uint64_t GetDeviceAddress() const override;

		RawPtr<RHIResource> GetResource() const { return m_resource; }
		bool IsTexelBufferView() const override;

		VT_NODISCARD VT_INLINE const BufferViewDesc& GetDesc() const override { return m_desc; }
		VT_NODISCARD VT_INLINE VkBufferView_T* GetTexelBufferView() const { return m_texelBufferView; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void CreateView();

		BufferViewDesc m_desc;
		RawPtr<RHIResource> m_resource;
		VkBufferView_T* m_texelBufferView = nullptr;
	};
}
