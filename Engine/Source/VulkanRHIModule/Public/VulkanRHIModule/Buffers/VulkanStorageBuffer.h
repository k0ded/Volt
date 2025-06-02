#pragma once

#include "VulkanRHIModule/Core.h"

#include <RHIModule/Buffers/StorageBuffer.h>

namespace Volt::RHI
{
	class Allocation;
	class GPUAllocator;

	class VulkanStorageBuffer : public StorageBuffer
	{
	public:
		VulkanStorageBuffer(const BufferDesc& desc, RefPtr<GPUAllocator> allocator = nullptr);
		~VulkanStorageBuffer() override;

		void Resize(const uint64_t size) override;
		void ResizeWithCount(const uint32_t count) override;
		const BufferDesc& GetDesc() const override;

		const uint64_t GetElementSize() const override;
		const uint32_t GetCount() const override;
		Handle<Allocation> GetAllocation() const override;

		void Unmap() override;
		void SetData(const void* data, const size_t size) override;
		void SetData(RefPtr<CommandBuffer> commandBuffer, const void* data, const size_t size) override;

		RefPtr<BufferView> GetView(const BufferViewDesc& desc) override;

		inline constexpr ResourceType GetType() const override { return ResourceType::StorageBuffer; }
		void SetName(const std::string& name) override;
		std::string_view GetName() const override;
		const uint64_t GetDeviceAddress() const override;
		const uint64_t GetByteSize() const override;

	protected:
		void* GetHandleImpl() const override;
		void* MapInternal() override;

	private:
		void Invalidate(const uint64_t byteSize);
		void Release();

		uint64_t m_byteSize = 0;
		BufferDesc m_desc;

		RefPtr<BufferView> m_view;
		Handle<Allocation> m_allocation;
		RawPtr<GPUAllocator> m_allocator;
	};
}
