#pragma once

#include "VulkanRHIModule/Core.h"

#include <RHIModule/Buffers/UniformBuffer.h>

#include <CoreUtilities/Allocators/Handle.h>

namespace Volt::RHI
{
	class Allocation;
	class VulkanUniformBuffer : public UniformBuffer
	{
	public:
		VulkanUniformBuffer(const UniformBufferDesc& desc, const void* initialData);
		~VulkanUniformBuffer() override;

		RefPtr<BufferView> GetView(const BufferViewDesc& desc) override;
		const uint32_t GetSize() const override;
		void SetData(const void* data, const uint32_t size) override;
		void Unmap() override;

		inline constexpr ResourceType GetType() const override { return ResourceType::UniformBuffer; }
		void SetName(const std::string& name) override;
		std::string_view GetName() const override;
		const uint64_t GetDeviceAddress() const override;
		const uint64_t GetByteSize() const override;

	protected:
		void* MapInternal(const uint32_t index) override;
		void* GetHandleImpl() const override;

	private:
		UniformBufferDesc m_desc;
		Handle<Allocation> m_allocation;
	};
}
