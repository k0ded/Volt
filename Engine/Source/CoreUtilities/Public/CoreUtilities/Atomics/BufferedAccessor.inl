#pragma once

template<typename T>
inline BufferedAccessor<T>::Writer::Writer(BufferedAccessor* accessor)
	: m_accessor(accessor)
{
	uint64_t expected = m_accessor->m_state.load(std::memory_order::relaxed);

	while (true)
	{
		if (expected & SwapPendingBit)
		{
			m_accessor->m_state.wait(expected, std::memory_order::acquire);
			expected = m_accessor->m_state.load(std::memory_order::relaxed);
			continue;
		}

		if (m_accessor->m_state.compare_exchange_weak(
			expected,
			expected + 1,
			std::memory_order::acquire,
			std::memory_order::relaxed))
		{
			break;
		}
	}

	m_value = m_accessor->m_writerValue.load(std::memory_order::acquire);
	VT_ASSERT(m_value);
}

template<typename T>
inline BufferedAccessor<T>::Writer::~Writer()
{
	const uint64_t prevValue = m_accessor->m_state.fetch_sub(1, std::memory_order::release);

	// Check if this is the last writer.
	if ((prevValue & WriterCountMask) == 1)
	{
		m_accessor->m_state.notify_all();
	}
}


template<typename T>
void BufferedAccessor<T>::Initialize(T* initialReader, T* initialWriter)
{
	m_readerValue = initialReader;
	m_writerValue = initialWriter;
}

template<typename T>
inline BufferedAccessor<T>::Writer BufferedAccessor<T>::GetWriter()
{
	return { this };
}

template<typename T>
inline T* BufferedAccessor<T>::SwapAndGetReader()
{
	VT_ASSERT(m_readerValue);

	m_state.fetch_or(SwapPendingBit, std::memory_order::acquire);
	
	while (true)
	{
		const uint64_t currentNumWriters = m_state.load(std::memory_order::acquire);
		if ((currentNumWriters & WriterCountMask) == 0)
		{
			break;
		}

		m_state.wait(currentNumWriters, std::memory_order::relaxed);
	}

	m_readerValue = m_writerValue.exchange(m_readerValue, std::memory_order::relaxed);

	// Notify writers that new value has been published.
	m_state.fetch_and(WriterCountMask, std::memory_order::release);
	m_state.notify_all();

	return m_readerValue;
}
