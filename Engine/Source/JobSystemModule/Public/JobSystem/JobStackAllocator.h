#pragma once

#include "JobSystem/FiberCommon.h"

#include <CoreUtilities/Containers/AtomicStack.h>

namespace Volt
{
	class JobStackAllocator
	{
	public:
		inline static constexpr uint32_t NumSmallStacks = 128;
		inline static constexpr uint32_t NumMediumStacks = 64;
		inline static constexpr uint32_t NumLargeStacks = 32;

		JobStackAllocator();
		~JobStackAllocator();

		FiberStack TryGetSmallStack();
		void FreeSmallStack(FiberStack stack);

		FiberStack TryGetMediumStack();
		void FreeMediumStack(FiberStack stack);

		FiberStack TryGetLargeStack();
		void FreeLargeStack(FiberStack stack);

	private:
		void Initialize();

		AtomicStack<FiberStack> m_smallStacks;
		AtomicStack<FiberStack> m_mediumStacks;
		AtomicStack<FiberStack> m_largeStacks;

		void* m_smallStackBase = nullptr;
		void* m_mediumStackBase = nullptr;
		void* m_largeStackBase = nullptr;
	};
}
