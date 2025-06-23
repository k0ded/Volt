#pragma once

#include <CoreUtilities/Containers/AtomicStack.h>
#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	template<typename Type, size_t Size>
	class JobAllocator
	{
	public:
		JobAllocator();

		Type* Allocate();
		void Free(Type* job);

	private:
		std::atomic<uint32_t> m_numAllocated;
		Array<Type, Size> m_allocator;

		AtomicStack<uint32_t, Size> m_availableStack;
	};

	template<typename Type, size_t Size>
	void JobAllocator<Type, Size>::Free(Type* valuePtr)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE_MSG(valuePtr >= &m_allocator[0] && valuePtr < &m_allocator[Size - 1], "Job does not belong to allocator!");
		
		const uint32_t index = static_cast<uint32_t>(std::distance(m_allocator.begin(), valuePtr));
		m_availableStack.Push(index);
	}

	template<typename Type, size_t Size>
	Type* JobAllocator<Type, Size>::Allocate()
	{
		VT_PROFILE_FUNCTION();
		uint32_t index;

		// Try to get a value from the available stack.
		if (!m_availableStack.Pop(index))
		{
			// Otherwise get a new one.
			index = m_numAllocated.fetch_add(1, std::memory_order::relaxed);
		}

		return &m_allocator[index];
	}

	template<typename Type, size_t Size>
	JobAllocator<Type, Size>::JobAllocator()
		: m_numAllocated(0)
	{}
}
