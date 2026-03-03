#pragma once

#include <RHIModule/Buffers/TransientBuffer.h>

struct VkBuffer_T;

namespace Volt::RHI
{
	class VulkanTransientBuffer final : public TransientBuffer
	{
	public:
		VulkanTransientBuffer(const BufferDesc& desc);
		~VulkanTransientBuffer() override;

		/*
		* TransientBuffer Interface
		*/
		void BindMemory(RefPtr<RHI::TransientHeap> heap, uint32_t pageIndex, uint64_t offset) override;

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
		uint64_t GetResourceByteSize() const override;

	protected:
		void* GetHandleImpl() const override;
		void* MapInternal() override;

	private:
		void CreateBuffer();

		BufferDesc m_desc;

		VkBuffer_T* m_bufferHandle;
		uint64_t m_deviceAddress = 0;
		MemoryRequirement m_memoryRequirements;
	};
}
