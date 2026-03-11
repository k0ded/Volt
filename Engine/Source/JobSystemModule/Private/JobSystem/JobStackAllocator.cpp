#include "jspch.h"

#include "JobSystem/JobStackAllocator.h"

#include <Volt-Platforms/Platform.h>

namespace Volt
{
	JobStackAllocator::JobStackAllocator()
	{
		Initialize();
	}

	JobStackAllocator::~JobStackAllocator()
	{
		if (m_smallStackBase)
		{
			PlatformMemory::FreeFiberStacks(m_smallStackBase);
		}

		if (m_mediumStackBase)
		{
			PlatformMemory::FreeFiberStacks(m_mediumStackBase);
		}

		if (m_largeStackBase)
		{
			PlatformMemory::FreeFiberStacks(m_largeStackBase);
		}
	}

	FiberStack JobStackAllocator::TryGetSmallStack()
	{
		FiberStack result;
		VT_MAYBE_UNUSED bool success = m_smallStacks.Pop(result);
		VT_ENSURE(success);

		return result;
	}

	void JobStackAllocator::FreeSmallStack(FiberStack stack)
	{
		VT_ASSERT(stack.GetStackSize() == FiberStackSize::Small);
		VT_MAYBE_UNUSED bool success = m_smallStacks.Push(stack);
		VT_ENSURE(success);
	}

	FiberStack JobStackAllocator::TryGetMediumStack()
	{
		FiberStack result;
		VT_MAYBE_UNUSED bool success = m_mediumStacks.Pop(result);
		VT_ENSURE(success);

		return result;
	}

	void JobStackAllocator::FreeMediumStack(FiberStack stack)
	{
		VT_ASSERT(stack.GetStackSize() == FiberStackSize::Medium);
		VT_MAYBE_UNUSED bool success = m_mediumStacks.Push(stack);
		VT_ENSURE(success);
	}

	FiberStack JobStackAllocator::TryGetLargeStack()
	{
		FiberStack result;
		VT_MAYBE_UNUSED bool success = m_largeStacks.Pop(result);
		VT_ENSURE(success);
	
		return result;
	}

	void JobStackAllocator::FreeLargeStack(FiberStack stack)
	{
		VT_ASSERT(stack.GetStackSize() == FiberStackSize::Large);
		VT_MAYBE_UNUSED bool success = m_largeStacks.Push(stack);
		VT_ENSURE(success);
	}

	void JobStackAllocator::Initialize()
	{
		{
			Vector<FiberStackDesc> stackDescs;
			m_smallStackBase = PlatformMemory::AllocateFiberStacks(std::to_underlying(FiberStackSize::Small), NumSmallStacks, stackDescs);

			// Initialize the stacks
			m_smallStacks.Allocate(NumSmallStacks);

			for (uint32_t i = 0; i < NumSmallStacks; ++i)
			{
				m_smallStacks.Push({ stackDescs[i].stackBase, stackDescs[i].guardBase, FiberStackSize::Small });
			}
		}

		{
			Vector<FiberStackDesc> stackDescs;
			m_mediumStackBase = PlatformMemory::AllocateFiberStacks(std::to_underlying(FiberStackSize::Medium), NumMediumStacks, stackDescs);

			// Initialize the stacks
			m_mediumStacks.Allocate(NumMediumStacks);

			for (uint32_t i = 0; i < NumMediumStacks; ++i)
			{
				m_mediumStacks.Push({ stackDescs[i].stackBase, stackDescs[i].guardBase, FiberStackSize::Medium });
			}
		}

		{
			Vector<FiberStackDesc> stackDescs;
			m_smallStackBase = PlatformMemory::AllocateFiberStacks(std::to_underlying(FiberStackSize::Large), NumLargeStacks, stackDescs);

			// Initialize the stacks
			m_largeStacks.Allocate(NumLargeStacks);

			for (uint32_t i = 0; i < NumLargeStacks; ++i)
			{
				m_largeStacks.Push({ stackDescs[i].stackBase, stackDescs[i].guardBase, FiberStackSize::Large });
			}
		}
	}
}
