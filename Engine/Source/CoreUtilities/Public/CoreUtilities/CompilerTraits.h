#pragma once

#include "CoreUtilities/Config.h"

// Static analasys assume
#if defined (_MSC_VER) && (_MSC_VER >= 1300)
	#define VT_ANALASYS_ASSUME(x) __analysis_assume(!!(x))
#else
	#define VT_ANALASYS_ASSUME(x)
#endif

// Inline
#ifdef VT_PLATFORM_WINDOWS
	#define VT_INLINE __forceinline
#else
	#define VT_INLINE inline
#endif

#if defined(_MSC_VER)
#define VT_DISABLE_WARNING(w) \
	__pragma(warning(push)) \
	__pragma(warning(disable:w))

#define VT_RESTORE_WARNING() \
	__pragma(warning(pop))

#else
#define VT_DISABLE_WARNING()
#define VT_RESTORE_WARNING()
#endif

// Optimize
#ifdef VT_ENABLE_OPTIMIZATION_TOGGLE
	#ifdef VT_PLATFORM_WINDOWS
		#define VT_OPTIMIZE_OFF __pragma(optimize("", off));
		#define VT_OPTIMIZE_ON __pragma(optimize("", on));
	#else
		#define VT_OPTIMIZE_OFF __pragma(optimize("", off));
		#define VT_OPTIMIZE_ON __pragma(optimize("", on));
	#endif
#else
	#define VT_OPTIMIZE_ON
	#define VT_OPTIMIZE_OFF
#endif

// Min alignment
#ifdef VT_PLATFORM_WINDOWS
	#define MIN_PLATFORM_ALIGNMENT 16ull
#else
	#error "Not defined!"
#endif

// Unused
template<typename T>
inline void VTBaseUnused(const volatile T& x) { (void)x; }
#define VT_UNUSED(x) VTBaseUnused(x)

// Debugbreak
#ifdef VT_PLATFORM_WINDOWS
	#define VT_DEBUGBREAK() __debugbreak()
#else
	#error "Not defined!"
#endif

// malloca
#ifdef VT_PLATFORM_WINDOWS
	#define VT_STACK_ALLOCATE(x) _malloca(x)
	#define VT_STACK_FREE(x) _freea(x)
#else
	#error "Not defined!"
#endif

// Barrier
#ifdef VT_PLATFORM_WINDOWS
	#define VT_COMPILER_BARRIER() _ReadWriteBarrier()
#else
	#error "Not defined!"
#endif

// Thread Pause
#ifdef VT_PLATFORM_WINDOWS
	#define VT_PAUSE_THREAD() _mm_pause()
#else
	#error "Not defined!"
#endif

#define VT_NODISCARD [[nodiscard]]
#define VT_MAYBE_UNUSED [[maybe_unused]]
#define VT_FALLTHROUGH [[fallthrough]]

#define VT_UNREACHABLE __assume(0)

#ifdef VT_PLATFORM_WINDOWS
	#define VT_RESTRICT __restrict
#else
	#error "Not defined!"
#endif
