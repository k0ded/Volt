#pragma once

#include "VulkanRHIModule/Core.h"
#include <RHIModule/Buffers/IndexBuffer.h>

#include <CoreUtilities/Allocators/Handle.h>

namespace Volt::RHI
{
	class Allocation;
	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(std::span<const uint32_t> indices);
		~VulkanIndexBuffer() override;

		const uint32_t GetCount() const override;
		inline constexpr ResourceType GetType() const override { return ResourceType::IndexBuffer; }
		void SetName(const std::string& name) override;
		std::string_view GetName() const override;
		const uint64_t GetDeviceAddress() const override;
		const uint64_t GetByteSize() const override;

	protected:
		void* GetHandleImpl() const override;

	private:
		void SetData(const void* data, const uint32_t size);

		std::string m_name;

		Handle<Allocation> m_allocation;
		uint32_t m_count = 0;
	};
}
