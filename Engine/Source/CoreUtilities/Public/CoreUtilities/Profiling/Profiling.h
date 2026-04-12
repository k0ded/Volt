#pragma once

#include <Tracy.hpp>

#define VT_PROFILING_METHOD_NONE 0
#define VT_PROFILING_METHOD_TRACY 2

#define VT_PROFILING_METHOD VT_PROFILING_METHOD_TRACY

#if VT_PROFILING_METHOD == VT_PROFILING_METHOD_TRACY

#define VT_PROFILE_FRAME_START(name) FrameMarkStart(name)
#define VT_PROFILE_FRAME_END(name) FrameMarkEnd(name)

#define VT_PROFILE_FUNCTION(...)  ZoneTransient(___tracy_scoped_zone, true)
#define VT_PROFILE_TAG(NAME, ...)
#define VT_PROFILE_SCOPE(NAME) ZoneTransientN(__tracyScope, NAME, true)
#define VT_PROFILE_THREAD(...) tracy::SetThreadName(__VA_ARGS__)
#define VT_PROFILE_CATEGORY(...)

#define VT_PROFILE_MESSAGE(MESSAGE) TracyMessageL(MESSAGE)
#define VT_PROFILE_ALLOC(ptr, size) TracySecureAllocS(ptr, size, 10)
#define VT_PROFILE_FREE(ptr) TracySecureFreeS(ptr, 10)

#define VT_PROFILE_DECLARE_MUTEX(type, name) TracyLockable(type, name)
#define VT_PROFILE_DECLARE_MUTEX_SHARED(type, name) TracySharedLockable(type, name)
#define VT_PROFILE_DECLARE_MUTEX_NAMED(type, name, desc) TracyLockableN(type, name, desc)
#define VT_PROFILE_DECLARE_MUTEX_SHARED_NAMED(type, name, desc) TracySharedLockableN(type, name, desc)
#define VT_PROFILE_LOCK_MARK(lockName) LockMark(lockName)

#if TRACY_FIBERS
#define VT_PROFILE_FIBER_ENTER(identifier) TracyFiberEnter(identifier)
#define VT_PROFILE_FIBER_LEAVE() TracyFiberLeave
#else
#define VT_PROFILE_FIBER_ENTER(identifier)
#define VT_PROFILE_FIBER_LEAVE()
#endif

#else

#define VT_PROFILE_FRAME_START(name)
#define VT_PROFILE_FRAME_END(name)
#define VT_PROFILE_FUNCTION(...)
#define VT_PROFILE_TAG(NAME, ...)
#define VT_PROFILE_SCOPE(NAME)
#define VT_PROFILE_THREAD(...)
#define VT_PROFILE_CATEGORY(...)

#define VT_PROFILE_EVENT(MESSAGE)

#define VT_PROFILE_ALLOC(ptr, size)
#define VT_PROFILE_FREE(ptr)

#define VT_PROFILE_DECLARE_MUTEX(type, name) type name
#define VT_PROFILE_DECLARE_MUTEX_SHARED(type, name) type name
#define VT_PROFILE_DECLARE_MUTEX_NAMED(type, name) type name
#define VT_PROFILE_DECLARE_MUTEX_SHARED_NAMED(type, name) type name
#define VT_PROFILE_LOCK_MARK(lockName)

#define VT_PROFILE_FIBER_ENTER(identifier)
#define VT_PROFILE_FIBER_LEAVE()
#endif
