#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Core/RHICommon.h"

#include <CoreUtilities/UUID.h>
#include <CoreUtilities/Containers/Vector.h>

namespace Volt::RHI
{
	inline static constexpr uint32_t MAX_PAGE_COUNT = 5;

	struct AllocationBlock
	{
		uint64_t size = 0;
		uint64_t offset = 0;
		uint32_t pageId = 0;
	};

	struct PageAllocation
	{
		void* handle = nullptr;
		uint64_t size = 0;
		uint64_t alignment = 0;

		uint64_t usedSize = 0;
		uint64_t tail = 0;

		Vector<AllocationBlock> availableBlocks;

		VT_NODISCARD VT_INLINE const uint64_t GetRemainingSize() const
		{
			return size - std::min(usedSize, size);
		}

		VT_NODISCARD VT_INLINE const uint64_t GetRemainingTailSize() const
		{
			return size - tail;
		}
	};

	class VTRHI_API Allocation
	{
	public:
		virtual ~Allocation() = default;

		template<typename T>
		constexpr T GetResourceHandle() const;

		template<typename T>
		constexpr T* Map();

		virtual void Unmap() = 0;
		virtual void Flush(uint64_t offset, uint64_t size) = 0;
		VT_NODISCARD virtual const UUID64 GetHeapID() const = 0;
		VT_NODISCARD virtual const uint64_t GetDeviceAddress() const = 0;
		VT_NODISCARD virtual const size_t GetHash() const = 0;
		VT_NODISCARD virtual const MemoryRequirement& GetMemoryRequirements() const = 0;
		VT_NODISCARD virtual std::string_view GetName() const = 0;

	protected:
		friend class GPUAllocator;

		virtual void* GetResourceHandleInternal() const = 0;
		virtual void* GetHandleImpl() const = 0;
		virtual void* MapInternal() = 0;

		Allocation() = default;
	};

	class VTRHI_API TransientAllocation : public Allocation
	{
	public:
		virtual ~TransientAllocation() override = default;

	protected:
		TransientAllocation() = default;
	};

	template<typename T>
	constexpr inline T Allocation::GetResourceHandle() const
	{
		return reinterpret_cast<T>(GetResourceHandleInternal());
	}

	template<typename T>
	constexpr inline T* Allocation::Map()
	{
		return reinterpret_cast<T*>(MapInternal());
	}
}
