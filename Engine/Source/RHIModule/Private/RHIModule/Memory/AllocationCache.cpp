#include "rhipch.h"

#include "RHIModule/Memory/AllocationCache.h"
#include "RHIModule/Memory/Allocation.h"

namespace Volt::RHI
{
	Handle<Allocation> AllocationCache::TryGetImageAllocationFromHash(const size_t hash)
	{
		std::scoped_lock lock{ m_imageAllocationMutex };
		VT_PROFILE_LOCK_MARK(m_imageAllocationMutex);

		for (int32_t i = static_cast<int32_t>(m_imageAllocations.size()) - 1; i >= 0; --i)
		{
			const auto alloc = m_imageAllocations.at(i);

			// We need to wait for one frame before reusing the allocation.
			if (alloc.allocation->GetHash() == hash && alloc.framesAlive > 1)
			{
				m_imageAllocations.erase_unsorted(m_imageAllocations.begin() + i);
				return alloc.allocation;
			}
		}

		return nullptr;
	}

	Handle<Allocation> AllocationCache::TryGetBufferAllocationFromHash(const size_t hash)
	{
		std::scoped_lock lock{ m_bufferAllocationMutex };
		VT_PROFILE_LOCK_MARK(m_bufferAllocationMutex);

		for (int32_t i = static_cast<int32_t>(m_bufferAllocations.size()) - 1; i >= 0; --i)
		{
			const auto alloc = m_bufferAllocations.at(i);

			// We need to wait for one frame before reusing the allocation.
			if (alloc.allocation->GetHash() == hash && alloc.framesAlive > 1)
			{
				m_bufferAllocations.erase_unsorted(m_bufferAllocations.begin() + i);
				return alloc.allocation;
			}
		}

		return nullptr;
	}

	void AllocationCache::QueueImageAllocationForRemoval(Handle<Allocation> alloc)
	{
		std::scoped_lock lock{ m_imageAllocationMutex };
		VT_PROFILE_LOCK_MARK(m_imageAllocationMutex);

		m_imageAllocations.emplace_back(alloc, 0);
	}

	void AllocationCache::QueueBufferAllocationForRemoval(Handle<Allocation> alloc)
	{
		std::scoped_lock lock{ m_bufferAllocationMutex };
		VT_PROFILE_LOCK_MARK(m_bufferAllocationMutex);

		m_bufferAllocations.emplace_back(alloc, 0);
	}

	AllocationsToRemove AllocationCache::UpdateAndGetAllocationsToDestroy()
	{
		constexpr size_t MAX_FRAMES_ALIVE = 3;
		
		AllocationsToRemove result{};

		{
			std::scoped_lock lock{ m_imageAllocationMutex };
			VT_PROFILE_LOCK_MARK(m_imageAllocationMutex);

			for (int32_t i = static_cast<int32_t>(m_imageAllocations.size()) - 1; i >= 0; --i)
			{
				const auto& alloc = m_imageAllocations.at(i);
				if (alloc.framesAlive >= MAX_FRAMES_ALIVE)
				{
					result.imageAllocations.push_back(alloc.allocation);
					m_imageAllocations.erase_unsorted(m_imageAllocations.begin() + i);
				}
			}
		}

		{
			std::scoped_lock lock{ m_bufferAllocationMutex };
			VT_PROFILE_LOCK_MARK(m_bufferAllocationMutex);

			for (int32_t i = static_cast<int32_t>(m_bufferAllocations.size()) - 1; i >= 0; --i)
			{
				const auto& alloc = m_bufferAllocations.at(i);
				if (alloc.framesAlive >= MAX_FRAMES_ALIVE)
				{
					result.bufferAllocations.push_back(alloc.allocation);
					m_bufferAllocations.erase_unsorted(m_bufferAllocations.begin() + i);
				}
			}
		}

		for (auto& allocInfo : m_imageAllocations)
		{
			allocInfo.framesAlive++;
		}

		for (auto& allocInfo : m_bufferAllocations)
		{
			allocInfo.framesAlive++;
		}

		return result;
	}
}
