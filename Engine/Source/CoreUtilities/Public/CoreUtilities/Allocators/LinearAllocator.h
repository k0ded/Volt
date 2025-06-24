#pragma once

#include "CoreUtilities/Allocators/HeapAllocator.h"
#include "CoreUtilities/VoltAssert.h"

#include <atomic>

template<size_t MaxByteSize, typename SecondaryAllocator = HeapAllocator>
class LinearAllocator
{
public:
	LinearAllocator()
	{ 
		m_dataBuffer = reinterpret_cast<uint8_t*>(m_allocator.Allocate(MaxByteSize, alignof(uint8_t)));
	}

	~LinearAllocator()
	{
		if (m_dataBuffer)
		{
			m_allocator.Free(m_dataBuffer);
		}
	}

	LinearAllocator(const LinearAllocator& other) noexcept
	{
		m_allocator = other.m_allocator;
		m_dataBuffer = reinterpret_cast<uint8_t*>(m_allocator.Allocate(MaxByteSize, alignof(uint8_t)));

		m_dataPointer.store(other.m_dataPointer.load());
		memcpy(m_dataBuffer, other.m_dataBuffer, MaxByteSize);
	}

	LinearAllocator(LinearAllocator&& other) noexcept
	{
		m_allocator = std::move(other.m_allocator);
		m_dataPointer.store(other.m_dataPointer.load());
		m_dataBuffer = std::move(other.m_dataBuffer);

		other.m_dataBuffer = nullptr;
	}

	LinearAllocator& operator=(const LinearAllocator& other) noexcept
	{
		m_allocator = other.m_allocator;
		m_dataPointer.store(other.m_dataPointer.load());
		memcpy(m_dataBuffer, other.m_dataBuffer, MaxByteSize);

		return *this;
	}

	LinearAllocator& operator=(LinearAllocator&& other) noexcept
	{
		m_allocator = std::move(other.m_allocator);
		m_dataPointer.store(other.m_dataPointer.load());
		m_dataBuffer = std::move(other.m_dataBuffer);

		other.m_dataBuffer = nullptr;

		return *this;
	}

	size_t GetAllocatedSize()
	{
		return m_dataPointer;
	}

	void* Allocate(size_t allocationSize)
	{
		size_t allocationOffset = m_dataPointer.fetch_add(allocationSize);
		VT_ENSURE(m_dataPointer <= MaxByteSize);

		return &m_dataBuffer[allocationOffset];
	}

	uint8_t* GetData() const
	{
		return m_dataBuffer;
	}

	void Reset()
	{
		m_dataPointer = 0;
	}

private:
	uint8_t* m_dataBuffer = nullptr;
	std::atomic_size_t m_dataPointer = 0;

	SecondaryAllocator m_allocator;
};
