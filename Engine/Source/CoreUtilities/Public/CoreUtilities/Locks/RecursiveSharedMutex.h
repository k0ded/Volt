#pragma once

#include "CoreUtilities/CompilerTraits.h"
#include "CoreUtilities/Containers/Map.h"

#include <shared_mutex>

class RecursiveSharedMutex
{
public:
	RecursiveSharedMutex() = default;

	RecursiveSharedMutex(const RecursiveSharedMutex&) = delete;
	RecursiveSharedMutex& operator=(const RecursiveSharedMutex&) = delete;

	VTCOREUTIL_API void lock();
	VTCOREUTIL_API bool try_lock();
	VTCOREUTIL_API void unlock();

	VTCOREUTIL_API void lock_shared();
	VTCOREUTIL_API bool try_lock_shared();
	VTCOREUTIL_API void unlock_shared();

private:
	struct DefaultZero
	{
		size_t value = 0;
	};

	bool IncrementSharedLock(std::thread::id threadId);
	bool DecrementSharedLock(std::thread::id threadId);

	std::shared_mutex m_sharedMutex;
	std::mutex m_sharedLockInserterMutex;

	Map<std::thread::id, DefaultZero> m_sharedLocks;
};
