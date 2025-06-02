#pragma once

#include "VulkanRHIModule/Core.h"
#include <RHIModule/Buffers/BufferView.h>

struct VkBufferView_T;

namespace Volt::RHI
{
	class VulkanBufferView : public BufferView
	{
	public:
		VulkanBufferView(const BufferViewDesc& specification);
		~VulkanBufferView() override;

		VT_NODISCARD const uint64_t GetDeviceAddress() const override;

		RHIResource* GetResource() const { return m_buffer; }
		bool IsTexelBufferView() const override;

		VT_NODISCARD VT_INLINE const BufferViewDesc& GetDesc() const { return m_desc; }
		VT_NODISCARD VT_INLINE VkBufferView_T* GetTexelBufferView() const { return m_texelBufferView; }

	protected:
		void* GetHandleImpl() const override;

	private:
		BufferViewDesc m_desc;
		RHIResource* m_buffer = nullptr;

		VkBufferView_T* m_texelBufferView = nullptr;
	};
}
