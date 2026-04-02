#include "cupch.h"
#include "VoltAssert.h"

#ifdef VT_PLATFORM_WINDOWS
	#if defined(_MSC_VER)
		#include <crtdbg.h>
	#endif

#include "CoreUtilities/Platform/Windows/VoltWindows.h"
#endif

void AssertionFailure(const char* expression)
{
#if defined(VT_ENABLE_ASSERTS) || defined(VT_ENABLE_ENSURES)
#ifdef VT_PLATFORM_WINDOWS
	printf("ASSERTION FAILURE: %s\n", expression);
	if (::IsDebuggerPresent())
	{
		OutputDebugStringA(expression);
	}
#else	
	printf("%s\n", expression);
#endif
#elif VT_ENABLE_ENSURES
	VT_UNUSED(expression);
#endif

	VT_DEBUGBREAK();

#ifdef VT_PLATFORM_WINDOWS
	if (!::IsDebuggerPresent())
#endif
	{
		std::exit(1);
	}
}

void AssertionFailure(StringView expression)
{
	AssertionFailure(expression.data());
}

bool CheckExpression(bool expression, const char* str)
{
	if (!expression)
	{
#if defined(VT_ENABLE_CHECKS)
#ifdef VT_PLATFORM_WINDOWS
		printf("CHECK FAILURE: %s\n", str);
		if (::IsDebuggerPresent())
		{
			OutputDebugStringA(str);
		}
#else	
		printf("%s\n", str);
#endif
#else
		VT_UNUSED(str);
#endif
#ifdef VT_PLATFORM_WINDOWS
		if (::IsDebuggerPresent())
#endif
		{
			VT_DEBUGBREAK();
		}
	}

	return expression;
}

bool CheckExpression(bool expression, StringView str)
{
	return CheckExpression(expression, str.data());
}
