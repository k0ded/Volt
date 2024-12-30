#pragma once

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/CompilerTraits.h>

#include <mutex>

template<typename Type, size_t MaxCount>
class ArenaAllocator
{
public:
	ArenaAllocator()
	{
		m_dataBuffer = new uint8_t[MaxCount * sizeof(Type)];
	}

	~ArenaAllocator()
	{
		delete[] m_dataBuffer;
	}

	size_t GetNumAllocations() const
	{
		return m_nextIndex - m_availableIndices.size();
	}

	template<typename... Args>
	Type* Allocate(Args&& ... args)
	{
		size_t newIndex = std::numeric_limits<size_t>::max();

		{
			std::scoped_lock lock{ m_mutex };
			if (!m_availableIndices.empty())
			{
				newIndex = m_availableIndices.back();
				m_availableIndices.pop_back();
			}
		}

		if (newIndex == std::numeric_limits<size_t>::max())
		{
			VT_ENSURE(m_nextIndex < MaxCount);
			newIndex = m_nextIndex++;
		}

		Type* newAllocation = ::new(&m_dataBuffer[newIndex * sizeof(Type)]) Type(std::forward<Args>(args)...);
		return newAllocation;
	}

	void Free(Type* allocation)
	{
		std::ptrdiff_t allocationIndex = allocation - reinterpret_cast<Type*>(m_dataBuffer);
		VT_ENSURE(allocationIndex < MaxCount);
		allocation->~Type();

		{
			std::scoped_lock lock{ m_mutex };
			m_availableIndices.emplace_back(allocationIndex);
		}
	}

	// Note: This is a slow operation!
	VT_NODISCARD Vector<Type*> GetActiveAllocations() const
	{
		Vector<Type*> result;
		result.reserve(m_nextIndex);

		for (size_t i = 0; i < m_nextIndex; i++)
		{
			if (auto it = std::ranges::find(m_availableIndices, i); it == m_availableIndices.end())
			{
				result.emplace_back(reinterpret_cast<Type*>(&m_dataBuffer[i * sizeof(Type)]));
			}
		}

		return result;
	}

private:
	std::mutex m_mutex;
	Vector<size_t> m_availableIndices;

	uint8_t* m_dataBuffer = nullptr;
	std::atomic_size_t m_nextIndex = 0;
};

