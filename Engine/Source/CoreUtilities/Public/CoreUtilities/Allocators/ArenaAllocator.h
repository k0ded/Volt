#pragma once

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Containers/AtomicStack.h>
#include <CoreUtilities/Containers/AtomicBitVector.h>
#include <CoreUtilities/CompilerTraits.h>
#include <CoreUtilities/Malloc.h>

#include <mutex>

template<typename Type, typename SecondaryAllocator = DefaultHeapAllocator>
class ArenaAllocator
{
public:
	ArenaAllocator()
	{}

	ArenaAllocator(ArenaAllocator&& other)
	{ 
		m_availableIndices = std::move(other.m_availableIndices);
		m_allocatedEntriesBitmask = std::move(other.m_allocatedEntriesBitmask);
		m_allocator = std::move(other.m_allocator);
		m_nextIndex = other.m_nextIndex.load();
		m_numMaxElements = other.m_numMaxElements;
		m_dataBuffer = other.m_dataBuffer;

		other.m_dataBuffer = nullptr;
		other.m_nextIndex = 0;
	}

	ArenaAllocator& operator=(ArenaAllocator&& other)
	{ 
		m_availableIndices = std::move(other.m_availableIndices);
		m_allocatedEntriesBitmask = std::move(other.m_allocatedEntriesBitmask);
		m_allocator = std::move(other.m_allocator);
		m_nextIndex = other.m_nextIndex.load();
		m_dataBuffer = other.m_dataBuffer;
		m_numMaxElements = other.m_numMaxElements;

		other.m_dataBuffer = nullptr;
		other.m_nextIndex = 0;

		return *this;
	}

	~ArenaAllocator()
	{
		auto activeAllocations = GetActiveAllocations();

		for (const auto& alloc : activeAllocations)
		{
			Free(alloc);
		}

		m_allocator.Free(m_dataBuffer);
	}

	void Reserve(const size_t numMaxElements)
	{
		VT_ENSURE_MSG(m_dataBuffer == nullptr, "An arena should only be allocated once!");

		m_dataBuffer = reinterpret_cast<uint8_t*>(m_allocator.Allocate(sizeof(Type) * numMaxElements, alignof(Type)));
		m_numMaxElements = numMaxElements;

		m_availableIndices.Allocate(numMaxElements);
		m_allocatedEntriesBitmask.Resize(numMaxElements);
	}

	VT_NODISCARD size_t GetNumAllocations() const
	{
		return std::min(m_nextIndex.load(std::memory_order::relaxed), m_numMaxElements) - m_availableIndices.Size();
	}

	VT_NODISCARD bool HasAvailableSlots() const
	{
		return !m_availableIndices.Empty() || m_nextIndex.load(std::memory_order::relaxed) < m_numMaxElements;
	}

	VT_NODISCARD size_t GetNumMaxAllocations() const
	{
		return m_numMaxElements;
	}

	VT_NODISCARD bool IsEmpty() const
	{
		return GetNumAllocations() == 0;
	}

	template<typename... Args>
	Type* Allocate(Args&& ... args)
	{
		size_t newIndex = std::numeric_limits<size_t>::max();

		if (!m_availableIndices.Pop(newIndex))
		{
			// Allocation failed, return nothing.
			newIndex = m_nextIndex.fetch_add(1);
			if (newIndex >= m_numMaxElements)
			{
				return nullptr;
			}
		}

		m_allocatedEntriesBitmask.SetBit(newIndex, true, std::memory_order::relaxed);

		void* dataPtr = &m_dataBuffer[newIndex * sizeof(Type)];
		Type* newAllocation = ::new(dataPtr) Type(std::forward<Args>(args)...);
		return newAllocation;
	}

	void Free(Type* allocation)
	{
		VT_ENSURE(IsPointerWithinArena(allocation));

		std::ptrdiff_t allocationIndex = allocation - reinterpret_cast<Type*>(m_dataBuffer);
		allocation->~Type();

		m_allocatedEntriesBitmask.SetBit(allocationIndex, false, std::memory_order::relaxed);
 		m_availableIndices.Push(allocationIndex);
	}

	bool IsPointerWithinArena(Type* ptr) const
	{
		if (reinterpret_cast<uint8_t*>(ptr) < m_dataBuffer)
		{
			return false;
		}

		std::ptrdiff_t allocationIndex = ptr - reinterpret_cast<Type*>(m_dataBuffer);
		if (allocationIndex >= m_numMaxElements)
		{
			return false;
		}

		return true;
	}

	// Note: This is a slow operation!
	Vector<Type*> GetActiveAllocations() const
	{
		const size_t numMaxItems = std::min(m_nextIndex.load(), m_numMaxElements);

		Vector<Type*> result;
		result.reserve(numMaxItems);

		Type* currentIt = reinterpret_cast<Type*>(const_cast<uint8_t*>(m_dataBuffer));

		for (size_t i = 0; i < numMaxItems; i++)
		{
			if (m_allocatedEntriesBitmask.IsBitSet(i, std::memory_order::relaxed))
			{
				result.emplace_back(currentIt);
			}

			currentIt++;
		}

		return result;
	}

	class Iterator
	{
	public:
		Iterator()
			: m_arenaAllocator(nullptr)
		{}

		Iterator(const ArenaAllocator& arenaAllocator)
			: m_arenaAllocator(&arenaAllocator)
		{
			// Store max index on create, to not move the end of the iterator during iteration.
			m_maxIndex = std::min(arenaAllocator.m_nextIndex.load(std::memory_order::relaxed), arenaAllocator.m_numMaxElements);

			// Find first allocation
			for (; m_currentIndex < m_maxIndex; ++m_currentIndex)
			{
				if (m_arenaAllocator->m_allocatedEntriesBitmask.IsBitSet(m_currentIndex, std::memory_order::relaxed))
				{
					break;
				}
			}
		}

		VT_INLINE void operator++()
		{
			// Find the next active allocation
			// Make sure we start at the next index.
			m_currentIndex++;

			for (; m_currentIndex < m_maxIndex; ++m_currentIndex)
			{
				if (m_arenaAllocator->m_allocatedEntriesBitmask.IsBitSet(m_currentIndex, std::memory_order::relaxed))
				{
					break;
				}
			}
		}

		VT_INLINE Type* operator->() const
		{
			VT_ENSURE(m_arenaAllocator->m_allocatedEntriesBitmask.IsBitSet(m_currentIndex, std::memory_order::relaxed));
			return reinterpret_cast<Type*>(&m_arenaAllocator->m_dataBuffer[m_currentIndex * sizeof(Type)]);
		}

		VT_INLINE Type* operator*() const
		{
			VT_ENSURE(m_arenaAllocator->m_allocatedEntriesBitmask.IsBitSet(m_currentIndex, std::memory_order::relaxed));
			return reinterpret_cast<Type*>(&m_arenaAllocator->m_dataBuffer[m_currentIndex * sizeof(Type)]);
		}

		VT_INLINE explicit operator bool() const
		{
			return m_arenaAllocator != nullptr && m_currentIndex < m_maxIndex;
		}

	private:
		const ArenaAllocator* m_arenaAllocator;
		size_t m_maxIndex = 0;
		size_t m_currentIndex = 0;
	};

private:
	SecondaryAllocator::template ForElementType<uint8_t> m_allocator;

	AtomicStack<size_t, SecondaryAllocator> m_availableIndices;
	AtomicBitVector<uint64_t, SecondaryAllocator> m_allocatedEntriesBitmask;
	std::atomic<size_t> m_nextIndex = 0;
	uint8_t* m_dataBuffer = nullptr;
	size_t m_numMaxElements = 0;
};

