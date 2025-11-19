#pragma once

#include <CoreUtilities/Containers/AtomicStack.h>
#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/Allocators/PagedArenaAllocator.h>

namespace Volt
{
	template<typename Type>
	class JobAllocator
	{
	public:
		JobAllocator();

		Type* Allocate();
		void Free(Type* job);

	private:
		PagedArenaAllocator<Type, 1024> m_allocator;
	};

	template<typename Type>
	void JobAllocator<Type>::Free(Type* valuePtr)
	{
		m_allocator.Free(valuePtr);
	}

	template<typename Type>
	Type* JobAllocator<Type>::Allocate()
	{
		return m_allocator.Allocate();
	}

	template<typename Type>
	JobAllocator<Type>::JobAllocator()
	{}
}
