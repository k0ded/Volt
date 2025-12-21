#pragma once

#include "Config.h"
#include "CompilerTraits.h"

#include <string_view>

VTCOREUTIL_API void AssertionFailure(const char* expression);
VTCOREUTIL_API void AssertionFailure(std::string_view expression);
VTCOREUTIL_API bool CheckExpression(bool expression, const char* str);
VTCOREUTIL_API bool CheckExpression(bool expression, std::string_view str);

#ifdef VT_ENABLE_ASSERTS

#define VT_ASSERT(expression) \
	do { \
		VT_ANALASYS_ASSUME(expression); \
		(void)((expression) || (AssertionFailure(#expression), 0)); \
	} while(false)

#define VT_ASSERT_MSG(expression, message) \
	do { \
		VT_ANALASYS_ASSUME(expression); \
		(void)((expression) || (AssertionFailure(message), 0)); \
	} while(false)
#else
#define VT_ASSERT(expression)
#define VT_ASSERT_MSG(expression, message)
#endif

#ifdef VT_ENABLE_ENSURES
#define VT_ENSURE(expression) \
	do { \
		VT_ANALASYS_ASSUME(expression); \
		if (!(expression)) { AssertionFailure(#expression); } \
	} while(false)

#define VT_ENSURE_MSG(expression, message) \
	do { \
		VT_ANALASYS_ASSUME(expression); \
		if (!(expression)) { AssertionFailure(message); } \
	} while(false)

#else
#define VT_ENSURE(expression)
#define VT_ENSURE_MSG(expression, message)
#endif

#ifdef VT_ENABLE_CHECKS
	#define VT_CHECK(expression) CheckExpression(expression, #expression)
	#define VT_CHECK_MSG(expression, message) CheckExpression(expression, message)
#else
	#define VT_CHECK(expression) expression
	#define VT_CHECK_MSG(expression, message) expression
#endif
