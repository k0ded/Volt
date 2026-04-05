#pragma once

template<typename MutexType>
class ScopedLock
{
public:
	ScopedLock(MutexType& mutex)
		: m_mutex(mutex)
	{
		m_mutex.lock();
	}

	~ScopedLock()
	{
		m_mutex.unlock();
	}

private:
	MutexType& m_mutex;
};
