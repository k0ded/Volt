#pragma once

#include <CoreUtilities/Core.h>

namespace Volt
{
	enum class FiberStackSize : uint32_t
	{
		None = 0,
		Small = 1u << 16u,
		Medium = 1u << 17u,
		Large = 1u << 19u
	};

	struct FiberStack
	{
		FiberStack()
			: m_stackBase(nullptr),
			m_guardBase(nullptr),
			m_stackSize(FiberStackSize::None)
		{}

		FiberStack(void* stackBase, void* guardBase, FiberStackSize stackSize)
			: m_stackBase(stackBase),
			m_guardBase(guardBase),
			m_stackSize(stackSize)
		{}

		VT_INLINE uint8_t* GetStackPointer()
		{
			uint8_t* stackPointer = reinterpret_cast<uint8_t*>(m_stackBase) + std::to_underlying(m_stackSize);

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
			return reinterpret_cast<uint8_t*>(m_stackBase) + std::to_underlying(m_stackSize);
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
			return m_guardBase != nullptr && m_stackBase != nullptr && std::to_underlying(m_stackSize) > 0;
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
