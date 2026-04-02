#pragma once

#include "Config.h"
#include "CompilerTraits.h"

#include <CoreUtilities/String/StringView.h>

VTCOREUTIL_API void AssertionFailure(const char* expression, int32_t line, const char* file);
VTCOREUTIL_API void AssertionFailure(StringView expression, int32_t line, const char* file);
VTCOREUTIL_API bool CheckExpression(bool expression, const char* str, int32_t line, const char* file);
VTCOREUTIL_API bool CheckExpression(bool expression, StringView str, int32_t line, const char* file);
VTCOREUTIL_API bool IsDebuggerAttached();
VTCOREUTIL_API void OutputToDebugConsole(const char* str);
VTCOREUTIL_API void ExitProgram();

#ifdef VT_ENABLE_ASSERTS

#define _VT_ASSERT_INTERNAL(expression, str) \
	do { \
		VT_ANALASYS_ASSUME(expression); \
		if (!(expression)) \
		{ \
			AssertionFailure(str, __LINE__, __FILE__); \
			if (IsDebuggerAttached()) { VT_DEBUGBREAK(); } \
			ExitProgram(); \
		} \
	} while(false)

#define VT_ASSERT(expression) \
	_VT_ASSERT_INTERNAL(expression, #expression)

#define VT_ASSERT_MSG(expression, message) \
	_VT_ASSERT_INTERNAL(expression, message)

#else

#define VT_ASSERT(expression)
#define VT_ASSERT_MSG(expression, message)

#endif

#ifdef VT_ENABLE_ENSURES

#define _VT_ENSURE_INTERNAL(expression, str) \
	do { \
		VT_ANALASYS_ASSUME(expression); \
		if (!(expression)) \
		{ \
			AssertionFailure(str, __LINE__, __FILE__); \
			if (IsDebuggerAttached()) { VT_DEBUGBREAK(); } \
		} \
	} while(false)

#define VT_ENSURE(expression) \
	_VT_ENSURE_INTERNAL(expression, #expression)

#define VT_ENSURE_MSG(expression, message) \
	_VT_ENSURE_INTERNAL(expression, message)

#define VT_ENSURE_NO_ENTRY() VT_ENSURE_MSG(false, "Code path should never be reached!")

#else

#define VT_ENSURE(expression)
#define VT_ENSURE_MSG(expression, message)

#endif

#define _VT_FATAL_INTERNAL(expression, str) \
	do { \
		VT_ANALASYS_ASSUME(expression); \
		if (!(expression)) \
		{ \
			AssertionFailure(str, __LINE__, __FILE__); \
			if (IsDebuggerAttached()) { VT_DEBUGBREAK(); } \
			ExitProgram(); \
		} \
	} while(false)

#define VT_FATAL(expression) \
	_VT_FATAL_INTERNAL(expression, #expression)

#define VT_FATAL_MSG(expression, message) \
	_VT_FATAL_INTERNAL(expression, message)

#ifdef VT_ENABLE_CHECKS
	#define _VT_CHECK_INTERNAL(expression, str) \
		[&]() -> bool \
		{ \
			bool _res = CheckExpression(expression, str, __LINE__, __FILE__); \
			if (!_res && IsDebuggerAttached()) { VT_DEBUGBREAK(); } \
			return _res; \
		}()

	#define VT_CHECK(expression) \
		_VT_CHECK_INTERNAL(expression, #expression)

	#define VT_CHECK_MSG(expression, message) \
		_VT_CHECK_INTERNAL(expression, message)
#else
	#define VT_CHECK(expression) expression
	#define VT_CHECK_MSG(expression, message) expression
#endif
