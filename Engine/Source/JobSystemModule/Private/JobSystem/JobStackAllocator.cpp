#include "jspch.h"

#include "JobSystem/JobStackAllocator.h"

#include <PlatformsModule/Platform.h>

namespace Volt
{
	VT_INLINE uint32_t GetNumStacks(FiberStackSize stackSize)
	{
		constexpr uint64_t TotalStackSize = 10 * 1024 * 1024;
		return static_cast<uint32_t>(TotalStackSize / GetFiberStackByteSize(stackSize));
	}

	JobStackAllocator::JobStackAllocator()
	{
		Initialize();
	}

	JobStackAllocator::~JobStackAllocator()
	{
		for (size_t i = 0; i < m_stackBaseAddresses.size(); ++i)
		{
			PlatformMemory::FreeFiberStacks(m_stackBaseAddresses[i]);
		}
	}

	bool JobStackAllocator::TryGetStack(FiberStackSize stackSize, FiberStack& outStack)
	{
		return m_stacks[std::to_underlying(stackSize)].Pop(outStack);
	}

	void JobStackAllocator::FreeStack(FiberStack stack)
	{
		VT_ENSURE(stack.IsValid());
		m_stacks[std::to_underlying(stack.GetStackSize())].Push(stack);
	}

	void JobStackAllocator::Initialize()
	{
		Vector<FiberStackDesc> stackDescs;

		for (size_t i = 0; i < m_stacks.size(); ++i)
		{
			FiberStackSize stackSize = static_cast<FiberStackSize>(i);

			const uint32_t numStacksToAllocate = GetNumStacks(stackSize);

			stackDescs.clear();

			m_stacks[i].Allocate(numStacksToAllocate);
			m_stackBaseAddresses[i] = PlatformMemory::AllocateFiberStacks(GetFiberStackByteSize(stackSize), numStacksToAllocate, stackDescs);

			for (uint32_t stackIdx = 0; stackIdx < numStacksToAllocate; ++stackIdx)
			{
				m_stacks[i].Push({ stackDescs[stackIdx].stackBase, stackDescs[stackIdx].guardBase, stackSize });
			}
		}
	}
}
