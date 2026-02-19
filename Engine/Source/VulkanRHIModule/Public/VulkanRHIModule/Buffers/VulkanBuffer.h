#pragma once

#include "VulkanRHIModule/Core.h"

#include <RHIModule/Buffers/Buffer.h>

#include <CoreUtilities/Containers/Map.h>

namespace Volt::RHI
{
	class Allocation;
	class GPUAllocator;

	class VulkanBuffer final : public Buffer
	{
	public:
		VulkanBuffer(const BufferDesc& desc, RefPtr<GPUAllocator> allocator = nullptr);
		~VulkanBuffer() override;

		/*
			Buffer Interface
		*/
		const BufferDesc& GetDesc() const override;
		uint64_t GetElementSize() const override;
		uint64_t GetNumElements() const override;
		RefPtr<BufferView> GetView(const BufferViewDesc& desc) override;
		void Unmap() override;

		/* 
			RHIResource Interface
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
		void Invalidate(const uint64_t byteSize);
		void Release();

		uint64_t m_byteSize = 0;
		BufferDesc m_desc;

		Handle<Allocation> m_allocation;
		RawPtr<GPUAllocator> m_allocator;
	};
}
