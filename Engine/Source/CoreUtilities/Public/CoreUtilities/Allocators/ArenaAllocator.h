#pragma once

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Containers/AtomicStack.h>
#include <CoreUtilities/CompilerTraits.h>
#include <CoreUtilities/Malloc.h>

#include <mutex>

template<typename Type>
class ArenaAllocator
{
public:
	ArenaAllocator()
	{}

	ArenaAllocator(const ArenaAllocator& other)
	{ 
		m_availableIndices = other.m_availableIndices;
		m_nextIndex = other.m_nextIndex.load();

		if (other.m_dataBuffer)
		{
			AllocateArena(other.m_numMaxElements);
			memcpy(m_dataBuffer, other.m_dataBuffer, sizeof(Type) * other.m_numMaxElements);
		}
	}

	ArenaAllocator(ArenaAllocator&& other)
	{ 
		m_availableIndices = std::move(other.m_availableIndices);
		m_nextIndex = other.m_nextIndex.load();
		m_numMaxElements = other.m_numMaxElements;
		m_dataBuffer = other.m_dataBuffer;

		other.m_dataBuffer = nullptr;
		other.m_nextIndex = 0;
	}

	ArenaAllocator& operator=(const ArenaAllocator& other)
	{ 
		m_availableIndices = other.m_availableIndices;
		m_nextIndex = other.m_nextIndex.load();

		if (other.m_dataBuffer)
		{
			AllocateArena(other.m_numMaxElements);
			memcpy(m_dataBuffer, other.m_dataBuffer, sizeof(Type) * other.m_numMaxElements);
		}

		return *this;
	}

	ArenaAllocator& operator=(ArenaAllocator&& other)
	{ 
		m_availableIndices = std::move(other.m_availableIndices);
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

		Memory::Free(m_dataBuffer);
	}

	void AllocateArena(const size_t numMaxElements)
	{
		VT_ENSURE_MSG(m_dataBuffer == nullptr, "An arena should only be allocated once!");

		m_dataBuffer = reinterpret_cast<uint8_t*>(Memory::Malloc(numMaxElements * sizeof(Type), alignof(Type)));
		m_numMaxElements = numMaxElements;

		m_availableIndices.Allocate(numMaxElements);
	}

	VT_NODISCARD size_t GetNumAllocations() const
	{
		return m_nextIndex.load(std::memory_order::relaxed) - m_availableIndices.Size();
	}

	VT_NODISCARD bool HasAvailableSlots() const
	{
		return !m_availableIndices.Empty() || m_nextIndex.load(std::memory_order::relaxed) < m_numMaxElements;
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
			newIndex = m_nextIndex.fetch_add(1);
			VT_ENSURE(newIndex < m_numMaxElements);
		}

		void* dataPtr = &m_dataBuffer[newIndex * sizeof(Type)];
		Type* newAllocation = ::new(dataPtr) Type(std::forward<Args>(args)...);
		return newAllocation;
	}

	void Free(Type* allocation)
	{
		VT_ENSURE(IsPointerWithinArena(allocation));

		std::ptrdiff_t allocationIndex = allocation - reinterpret_cast<Type*>(m_dataBuffer);
		allocation->~Type();

		// Set these to zero, to "mark" the allocation as unused.
		memset(allocation, 0, std::min(sizeof(Type), 8ull));

 		m_availableIndices.Push(allocationIndex);
	}

	bool IsPointerWithinArena(Type* ptr)
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
		Vector<Type*> result;
		result.reserve(m_nextIndex);

		Type* currentIt = reinterpret_cast<Type*>(const_cast<uint8_t*>(m_dataBuffer));

		for (size_t i = 0; i < m_nextIndex; i++)
		{
			if (*reinterpret_cast<size_t*>(currentIt) != 0)
			{
				result.emplace_back(currentIt);
			}

			currentIt++;
		}

		return result;
	}

private:
	AtomicStack<size_t> m_availableIndices;
	std::atomic<size_t> m_nextIndex = 0;
	uint8_t* m_dataBuffer = nullptr;
	size_t m_numMaxElements = 0;
};

