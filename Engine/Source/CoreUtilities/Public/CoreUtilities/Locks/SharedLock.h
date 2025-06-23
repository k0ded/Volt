#pragma once

template<typename MutexType>
class SharedLock
{
public:
	SharedLock(MutexType& mutex)
		: m_mutex(mutex)
	{
		m_mutex.LockShared();
	}

	~SharedLock()
	{
		m_mutex.UnlockShared();
	}

private:
	MutexType& m_mutex;
};
