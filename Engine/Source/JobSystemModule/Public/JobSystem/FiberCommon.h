#pragma once

#include <CoreUtilities/VoltAssert.h>

namespace Volt
{
	enum class FiberStackSize : uint8_t
	{
		KB16 = 0, //1u << 14u,
		KB32, //1u << 15u,
		KB64, //1u << 16u,
		KB128, //1u << 17u,
		KB512, //1u << 19u,
		Num,

		Invalid
	};

	VT_INLINE uint64_t GetFiberStackByteSize(FiberStackSize stackSize)
	{
		switch (stackSize)
		{
			case Volt::FiberStackSize::KB16: return 1ull << 14ull;
			case Volt::FiberStackSize::KB32: return 1ull << 15ull;
			case Volt::FiberStackSize::KB64: return 1ull << 16ull;
			case Volt::FiberStackSize::KB128: return 1ull << 17ull;
			case Volt::FiberStackSize::KB512: return 1ull << 19ull;
		}
		VT_ENSURE_NO_ENTRY();
		return 1;
	}

	struct FiberStack
	{
		FiberStack()
			: m_stackBase(nullptr),
			m_guardBase(nullptr),
			m_stackSize(FiberStackSize::Invalid)
		{}

		FiberStack(void* stackBase, void* guardBase, FiberStackSize stackSize)
			: m_stackBase(stackBase),
			m_guardBase(guardBase),
			m_stackSize(stackSize)
		{}

		VT_INLINE uint8_t* GetStackPointer()
		{
			uint8_t* stackPointer = reinterpret_cast<uint8_t*>(m_stackBase) + GetFiberStackByteSize(m_stackSize);

			// Align stack ponter to 16 bytes.
			stackPointer = (uint8_t*)((uintptr_t)stackPointer & ~0xF);

			// Space for return address
			stackPointer -= 8;

			// Make space for 32 bytes at bottom of stack.
			stackPointer -= 32;

			return stackPointer;
		}

		VT_INLINE uint8_t* GetStackBase()
		{
			return reinterpret_cast<uint8_t*>(m_stackBase) + GetFiberStackByteSize(m_stackSize);
		}

		VT_INLINE uint8_t* GetStackLimit()
		{
			uint8_t* stackLimit = reinterpret_cast<uint8_t*>(m_stackBase);
			return stackLimit;
		}

		VT_INLINE uint8_t* GetDeallocationStack()
		{
			return reinterpret_cast<uint8_t*>(m_guardBase);
		}

		VT_INLINE bool IsValid() const
		{
			return m_guardBase != nullptr && m_stackBase != nullptr && m_stackSize != FiberStackSize::Invalid;
		}

		VT_INLINE FiberStackSize GetStackSize() const
		{
			return m_stackSize;
		}

	private:
		void* m_stackBase;
		void* m_guardBase;
		FiberStackSize m_stackSize;
	};
}
