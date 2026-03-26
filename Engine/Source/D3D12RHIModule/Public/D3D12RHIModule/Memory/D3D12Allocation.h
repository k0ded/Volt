#pragma once

#include "D3D12RHIModule/Common/ComPtr.h"

#include <RHIModule/Memory/Allocation.h>

struct ID3D12Resource;
namespace D3D12MA
{
	class Allocation;
}

namespace Volt::RHI
{
	class D3D12ImageAllocation final : public Allocation
	{
	public:
		D3D12ImageAllocation(const size_t hash, const std::string& name);
		~D3D12ImageAllocation() override = default;

		void Unmap() override;
		void Flush(uint64_t offset, uint64_t size) override;
		VT_NODISCARD VT_INLINE const UUID64 GetHeapID() const override { return 0; }
		VT_NODISCARD const uint64_t GetDeviceAddress() const override;
		VT_NODISCARD VT_INLINE const size_t GetHash() const override { return m_allocationHash; }
		VT_NODISCARD VT_INLINE const MemoryRequirement& GetMemoryRequirements() const override { return m_memoryRequirements; }
		VT_NODISCARD VT_INLINE StringView GetName() const override { return m_name; }

	protected:
		void* GetResourceHandleInternal() const override;
		void* MapInternal() override;

	private:
		friend class D3D12DefaultGPUAllocator;

		void* GetHandleImpl() const override;

		std::string m_name;

		ID3D12Resource* m_resource = nullptr;
		D3D12MA::Allocation* m_allocation = nullptr;
		size_t m_allocationHash = 0;
		MemoryRequirement m_memoryRequirements;
	};

	class D3D12BufferAllocation final : public Allocation
	{
	public:
		D3D12BufferAllocation(const size_t hash, const std::string& name);
		~D3D12BufferAllocation() override = default;

		void Unmap() override;
		void Flush(uint64_t offset, uint64_t size) override;
		VT_NODISCARD VT_INLINE const UUID64 GetHeapID() const override { return 0; }
		VT_NODISCARD const uint64_t GetDeviceAddress() const override;
		VT_NODISCARD VT_INLINE const size_t GetHash() const override { return m_allocationHash; }
		VT_NODISCARD VT_INLINE const MemoryRequirement& GetMemoryRequirements() const override { return m_memoryRequirements; }
		VT_NODISCARD VT_INLINE StringView GetName() const override { return m_name; }

	protected:
		void* GetResourceHandleInternal() const override;
		void* MapInternal() override;

	private:
		friend class D3D12DefaultGPUAllocator;

		void* GetHandleImpl() const override;

		std::string m_name;

		ID3D12Resource* m_resource = nullptr;
		D3D12MA::Allocation* m_allocation = nullptr;
		size_t m_allocationHash = 0;
		MemoryRequirement m_memoryRequirements;
	};

	class D3D12TransientBufferAllocation : public Allocation
	{
	public:
		D3D12TransientBufferAllocation(const size_t hash, const std::string& name);
		~D3D12TransientBufferAllocation() override = default;

		void Unmap() override;
		void Flush(uint64_t offset, uint64_t size) override;
		VT_NODISCARD VT_INLINE const UUID64 GetHeapID() const override { return m_heapId; }
		VT_NODISCARD const uint64_t GetDeviceAddress() const override;
		VT_NODISCARD VT_INLINE const size_t GetHash() const override { return m_allocationHash; }
		VT_NODISCARD VT_INLINE const MemoryRequirement& GetMemoryRequirements() const override { return m_memoryRequirements; }
		VT_NODISCARD VT_INLINE StringView GetName() const override { return m_name; }

	protected:
		void* GetResourceHandleInternal() const override;
		void* MapInternal() override;

		void* GetHandleImpl() const override;

	private:
		friend class D3D12TransientHeap;

		std::string m_name;

		ID3D12Resource* m_resource = nullptr;
		size_t m_allocationHash = 0;
		MemoryRequirement m_memoryRequirements;

		AllocationBlock m_allocationBlock{};
		UUID64 m_heapId = 0;
	};

	class D3D12TransientImageAllocation : public Allocation
	{
	public:
		D3D12TransientImageAllocation(const size_t hash, const std::string& name);
		~D3D12TransientImageAllocation() override = default;

		void Unmap() override;
		void Flush(uint64_t offset, uint64_t size) override;
		VT_NODISCARD VT_INLINE const UUID64 GetHeapID() const override { return m_heapId; }
		VT_NODISCARD const uint64_t GetDeviceAddress() const override;
		VT_NODISCARD VT_INLINE const size_t GetHash() const override { return m_allocationHash; }
		VT_NODISCARD VT_INLINE const MemoryRequirement& GetMemoryRequirements() const override { return m_memoryRequirements; }
		VT_NODISCARD VT_INLINE StringView GetName() const override { return m_name; }

	protected:
		void* GetResourceHandleInternal() const override;
		void* MapInternal() override;

		void* GetHandleImpl() const override;

	private:
		friend class D3D12TransientHeap;

		std::string m_name;

		ID3D12Resource* m_resource = nullptr;
		size_t m_allocationHash = 0;
		MemoryRequirement m_memoryRequirements;

		AllocationBlock m_allocationBlock{};
		UUID64 m_heapId = 0;
	};
}
