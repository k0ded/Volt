#include "cupch.h"
#include "VoltAssert.h"

#include "CoreUtilities/String/VoltString.h"
#include "CoreUtilities/String/StringFormat.h"

#ifdef VT_PLATFORM_WINDOWS
	#if defined(_MSC_VER)
		#include <crtdbg.h>
	#endif

#include "CoreUtilities/Platform/Windows/VoltWindows.h"
#endif

void AssertionFailure(const char* expression, int32_t line, const char* file)
{
#if defined(VT_ENABLE_ASSERTS) || defined(VT_ENABLE_ENSURES)
	String tempString = FormatString("ASSERTION FAILURE: {} in {} at line {}\n", expression, file, line);
	printf("%s", tempString.c_str());

	if (IsDebuggerAttached())
	{
		OutputToDebugConsole(tempString.c_str());
	}
#else
	VT_UNUSED(expression);
	VT_UNUSED(line);
	VT_UNUSED(file);
#endif
}

void AssertionFailure(StringView expression, int32_t line, const char* file)
{
	AssertionFailure(expression.data(), line, file);
}

bool CheckExpression(bool expression, const char* str, int32_t line, const char* file)
{
	if (!expression)
	{
#if defined(VT_ENABLE_CHECKS)
		String tempString = FormatString("CHECK FAILURE: {} in {} at line {}\n", expression, file, line);
		printf("%s", tempString.c_str());

		if (IsDebuggerAttached())
		{
			OutputToDebugConsole(tempString.c_str());
		}
#else
		VT_UNUSED(expression);
		VT_UNUSED(str);
		VT_UNUSED(line);
		VT_UNUSED(file);
#endif
	}

	return expression;
}

bool CheckExpression(bool expression, StringView str, int32_t line, const char* file)
{
	return CheckExpression(expression, str.data(), line, file);
}

bool IsDebuggerAttached()
{
#ifdef VT_PLATFORM_WINDOWS
	return ::IsDebuggerPresent();
#else
	return false;
#endif
}

void OutputToDebugConsole(const char* str)
{
#ifdef VT_PLATFORM_WINDOWS
	OutputDebugStringA(str);
#endif
}

void ExitProgram()
{
	if (!IsDebuggerAttached())
	{
		std::exit(1);
	}
}
