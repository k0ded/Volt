#pragma once

#include "JobSystem/FiberCommon.h"

#include <CoreUtilities/Containers/AtomicStack.h>

namespace Volt
{
	class JobStackAllocator
	{
	public:
		inline static constexpr uint32_t NumSmallStacks = 1024;
		inline static constexpr uint32_t NumMediumStacks = 64;
		inline static constexpr uint32_t NumLargeStacks = 32;

		JobStackAllocator();
		~JobStackAllocator();

		bool TryGetStack(FiberStackSize stackSize, FiberStack& outStack);
		void FreeStack(FiberStack stack);

	private:
		void Initialize();

		Array<AtomicStack<FiberStack>, std::to_underlying(FiberStackSize::Num)> m_stacks;
		Array<void*, std::to_underlying(FiberStackSize::Num)> m_stackBaseAddresses;
	};
}
