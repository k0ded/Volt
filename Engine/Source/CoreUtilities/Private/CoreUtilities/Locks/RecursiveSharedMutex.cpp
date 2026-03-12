#include "cupch.h"

#include "CoreUtilities/Locks/RecursiveSharedMutex.h"

void RecursiveSharedMutex::lock()
{
	m_sharedMutex.lock();
}

bool RecursiveSharedMutex::try_lock()
{
	return m_sharedMutex.try_lock();
}

void RecursiveSharedMutex::unlock()
{
	m_sharedMutex.unlock();
}

void RecursiveSharedMutex::lock_shared()
{
	if (IncrementSharedLock(std::this_thread::get_id()))
	{
		m_sharedMutex.lock_shared();
	}
}

bool RecursiveSharedMutex::try_lock_shared()
{
	bool successful = false;

	bool shouldTryLock = IncrementSharedLock(std::this_thread::get_id());
	if (shouldTryLock)
	{
		successful = m_sharedMutex.try_lock_shared();
	}

	// Locking failed, make sure the counter is decremented again.
	if (shouldTryLock && !successful)
	{
		DecrementSharedLock(std::this_thread::get_id());
	}

	// If we didn't try to lock, the thread already holds a lock, and should return true.
	return (shouldTryLock == false) ? true : successful;
}

void RecursiveSharedMutex::unlock_shared()
{
	if (DecrementSharedLock(std::this_thread::get_id()))
	{
		m_sharedMutex.unlock_shared();
	}
}

bool RecursiveSharedMutex::IncrementSharedLock(std::thread::id threadId)
{
	std::scoped_lock lock{ m_sharedLockInserterMutex };
	size_t prevVal = m_sharedLocks[threadId].value++;
	
	return prevVal == 0;
}

bool RecursiveSharedMutex::DecrementSharedLock(std::thread::id threadId)
{
	std::scoped_lock lock{ m_sharedLockInserterMutex };
	size_t prevVal = m_sharedLocks[threadId].value--;

	if (m_sharedLocks[threadId].value == 0)
	{
		m_sharedLocks.erase(threadId);
	}

	return prevVal == 1;
}
