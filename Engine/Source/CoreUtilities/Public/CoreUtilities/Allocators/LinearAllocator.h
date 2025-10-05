#pragma once

#include "CoreUtilities/Allocators/ContainerAllocators.h"
#include "CoreUtilities/VoltAssert.h"

#include <atomic>

template<typename SecondaryAllocator = DefaultHeapAllocator>
class LinearAllocator
{
public:
	LinearAllocator()
	{}

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
		m_dataBuffer = reinterpret_cast<uint8_t*>(m_allocator.Allocate(other.m_size, 0));

		m_dataPointer.store(other.m_dataPointer.load());
		memcpy(m_dataBuffer, other.m_dataBuffer, other.m_size);
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
		m_dataBuffer = reinterpret_cast<uint8_t*>(m_allocator.Allocate(other.m_size, 0));

		m_dataPointer.store(other.m_dataPointer.load());
		memcpy(m_dataBuffer, other.m_dataBuffer, other.m_size);

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
		VT_ENSURE(m_dataPointer <= m_size);
		 
		return &m_dataBuffer[allocationOffset];
	}

	void Reserve(size_t size)
	{
		VT_ENSURE(!m_dataBuffer);
		m_dataBuffer = reinterpret_cast<uint8_t*>(m_allocator.Allocate(size, 0));
		m_size = size;
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
	size_t m_size = 0;
	std::atomic_size_t m_dataPointer = 0;

	SecondaryAllocator::template ForElementType<uint8_t> m_allocator;
};
