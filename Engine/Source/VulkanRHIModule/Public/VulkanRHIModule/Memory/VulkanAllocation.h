#pragma once

#include <RHIModule/Memory/Allocation.h>

struct VmaAllocation_T;
struct VkBuffer_T;
struct VkImage_T;
struct VkDeviceMemory_T;

namespace Volt::RHI
{
	class VulkanImageAllocation final : public Allocation
	{
	public:
		VulkanImageAllocation(const size_t hash, const String& name);
		~VulkanImageAllocation() override = default;

		void Unmap() override;
		void Flush(uint64_t offset, uint64_t size) override;
		VT_NODISCARD VT_INLINE const UUID64 GetHeapID() const override { return 0; }
		VT_NODISCARD const uint64_t GetDeviceAddress() const override;
		VT_NODISCARD VT_INLINE const size_t GetHash() const override { return m_allocationHash; }
		VT_NODISCARD VT_INLINE const MemoryRequirement& GetMemoryRequirements() const override { return m_memoryRequirement; }
		VT_NODISCARD VT_INLINE StringView GetName() const override { return m_name; }

	protected:
		void* GetResourceHandleInternal() const override;
		void* MapInternal() override;

	private:
		friend class VulkanDefaultGPUAllocator;

		void* GetHandleImpl() const override;

		String m_name;

		VkImage_T* m_resource = nullptr;
		VmaAllocation_T* m_allocation = nullptr;
		MemoryRequirement m_memoryRequirement;
		size_t m_allocationHash = 0;
	};

	class VulkanBufferAllocation final : public Allocation
	{
	public:
		VulkanBufferAllocation(const size_t hash, const String& name);
		~VulkanBufferAllocation() override = default;

		void Unmap() override;
		void Flush(uint64_t offset, uint64_t size) override;
		VT_NODISCARD VT_INLINE const UUID64 GetHeapID() const override { return 0; }
		VT_NODISCARD const uint64_t GetDeviceAddress() const override;
		VT_NODISCARD VT_INLINE const size_t GetHash() const override { return m_allocationHash; }
		VT_NODISCARD VT_INLINE const MemoryRequirement& GetMemoryRequirements() const override { return m_memoryRequirement; }
		VT_NODISCARD VT_INLINE StringView GetName() const override { return m_name; }

	protected:
		void* GetResourceHandleInternal() const override;
		void* MapInternal() override;

	private:
		friend class VulkanDefaultGPUAllocator;

		void* GetHandleImpl() const override;

		String m_name;

		VkBuffer_T* m_resource = nullptr;
		VmaAllocation_T* m_allocation = nullptr;
		MemoryRequirement m_memoryRequirement;
		size_t m_allocationHash = 0;
	};

	class VulkanTransientBufferAllocation : public Allocation
	{
	public:
		VulkanTransientBufferAllocation(const size_t hash, const String& name);
		~VulkanTransientBufferAllocation() override = default;

		void Unmap() override;
		void Flush(uint64_t offset, uint64_t size) override;
		VT_NODISCARD VT_INLINE const UUID64 GetHeapID() const override { return m_heapId; }
		VT_NODISCARD const uint64_t GetDeviceAddress() const override;
		VT_NODISCARD VT_INLINE const size_t GetHash() const override { return m_allocationHash; }
		VT_NODISCARD VT_INLINE const MemoryRequirement& GetMemoryRequirements() const override { return m_memoryRequirement; }
		VT_NODISCARD VT_INLINE StringView GetName() const override { return m_name; }

	protected:
		void* GetResourceHandleInternal() const override;
		void* MapInternal() override;

		void* GetHandleImpl() const override;

	private:
		friend class VulkanTransientHeap;

		String m_name;

		VkBuffer_T* m_resource = nullptr;
		VkDeviceMemory_T* m_memoryHandle = nullptr;
		size_t m_allocationHash = 0;
		MemoryRequirement m_memoryRequirement;

		AllocationBlock m_allocationBlock{};
		UUID64 m_heapId = 0;
	};

	class VulkanTransientImageAllocation : public Allocation
	{
	public:
		VulkanTransientImageAllocation(const size_t hash, const String& name);
		~VulkanTransientImageAllocation() override = default;

		void Unmap() override;
		void Flush(uint64_t offset, uint64_t size) override;
		VT_NODISCARD VT_INLINE const UUID64 GetHeapID() const override { return m_heapId; }
		VT_NODISCARD const uint64_t GetDeviceAddress() const override;
		VT_NODISCARD VT_INLINE const size_t GetHash() const override { return m_allocationHash; }
		VT_NODISCARD VT_INLINE const MemoryRequirement& GetMemoryRequirements() const override { return m_memoryRequirement; }
		VT_NODISCARD VT_INLINE StringView GetName() const override { return m_name; }

	protected:
		void* GetResourceHandleInternal() const override;
		void* MapInternal() override;

		void* GetHandleImpl() const override;

	private:
		friend class VulkanTransientHeap;

		String m_name;

		VkImage_T* m_resource = nullptr;
		VkDeviceMemory_T* m_memoryHandle = nullptr;
		size_t m_allocationHash = 0;
		MemoryRequirement m_memoryRequirement;

		AllocationBlock m_allocationBlock{};
		UUID64 m_heapId = 0;
	};
}
