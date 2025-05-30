#pragma once

#include "VulkanRHIModule/Core.h"
#include <RHIModule/Buffers/BufferView.h>

struct VkBuffer_T;

namespace Volt::RHI
{
	class VulkanBufferView : public BufferView
	{
	public:
		VulkanBufferView(const BufferViewDesc& specification);
		~VulkanBufferView() override = default;

		VT_NODISCARD const uint64_t GetDeviceAddress() const override;

		RHIResource* GetResource() const { return m_buffer; }

		VT_NODISCARD VT_INLINE const BufferViewDesc& GetDesc() const { return m_desc; }

	protected:
		void* GetHandleImpl() const override;

	private:
		BufferViewDesc m_desc;
		RHIResource* m_buffer = nullptr;
	};
}
