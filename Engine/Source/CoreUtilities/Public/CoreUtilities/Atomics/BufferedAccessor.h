#pragma once

#include <atomic>

template<typename T>
class BufferedAccessor
{
public:
	class Writer
	{
	public:
		Writer(BufferedAccessor* accessor);
		~Writer();

		VT_INLINE T& Get() { return *m_value; }

	private:
		BufferedAccessor* m_accessor;
		T* m_value = nullptr;
	};

	BufferedAccessor() = default;

	void Initialize(T* initialReader, T* initialWriter);

	Writer GetWriter();
	T* SwapAndGetReader();

private:
	inline static constexpr uint64_t SwapPendingBit = 1ull << 63ull;
	inline static constexpr uint64_t WriterCountMask = ~SwapPendingBit;

	std::atomic<uint64_t> m_state = 0;
	T* m_readerValue = nullptr;
	alignas(std::hardware_destructive_interference_size) std::atomic<T*> m_writerValue = nullptr;
};

#include "CoreUtilities/Atomics/BufferedAccessor.inl"
