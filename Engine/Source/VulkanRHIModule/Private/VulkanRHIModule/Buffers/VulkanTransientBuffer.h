#pragma once

#include <RHIModule/Buffers/TransientBuffer.h>

namespace Volt::RHI
{
	class VulkanTransientBuffer final : public TransientBuffer
	{
	public:
		VulkanTransientBuffer(const BufferDesc& desc);
		~VulkanTransientBuffer() override;

		/*
		* Buffer Interface
		*/
		const BufferDesc& GetDesc() const override;
		uint64_t GetElementSize() const override;
		uint64_t GetNumElements() const override;
		RefPtr<BufferView> GetView(const BufferViewDesc& desc) override;
		void Unmap() override;

		/*
		* RHIResource Interface
		*/
		inline constexpr ResourceType GetType() const override { return ResourceType::Buffer; }
		void SetName(const std::string& name) override;
		std::string_view GetName() const override;
		uint64_t GetDeviceAddress() const override;
		const MemoryRequirement& GetMemoryRequirements() const override;

	protected:
		void* GetHandleImpl() const override;
		void* MapInternal() override;

	private:
		void CreateBuffer();

		BufferDesc m_desc;
		Handle<Allocation> m_allocation;
	};
}
