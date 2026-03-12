#pragma once

template<typename MutexType>
class ScopedLock
{
public:
	ScopedLock(MutexType& mutex)
		: m_mutex(mutex)
	{
		m_mutex.Lock();
	}

	~ScopedLock()
	{
		m_mutex.Unlock();
	}

private:
	MutexType& m_mutex;
};
