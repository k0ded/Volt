#pragma once

#include "VulkanRHIModule/Core.h"

#include <RHIModule/Buffers/UniformBuffer.h>

#include <CoreUtilities/Allocators/Handle.h>

namespace Volt::RHI
{
	class Allocation;
	class VulkanUniformBuffer final : public UniformBuffer
	{
	public:
		VulkanUniformBuffer(const UniformBufferDesc& desc, const void* initialData);
		~VulkanUniformBuffer() override;

		/*
			Uniform Buffer Interface
		*/
		RefPtr<BufferView> GetView(const BufferViewDesc& desc) override;
		uint64_t GetSize() const override;
		void Unmap() override;

		/*
			RHIResource Interface
		*/
		inline constexpr ResourceType GetType() const override { return ResourceType::UniformBuffer; }
		void SetName(const std::string& name) override;
		std::string_view GetName() const override;
		uint64_t GetDeviceAddress() const override;
		const MemoryRequirement& GetMemoryRequirements() const override;
		uint64_t GetResourceByteSize() const override;

	protected:
		void* MapInternal() override;
		void* GetHandleImpl() const override;

	private:
		UniformBufferDesc m_desc;
		MemoryRequirement m_memoryRequirements;
		Handle<Allocation> m_allocation;
	};
}
