#pragma once

#include <CoreUtilities/Containers/AtomicStack.h>
#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>

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
		PagedAtomicArenaAllocator<Type, 1024, DefaultHeapAllocator, true> m_allocator;
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
