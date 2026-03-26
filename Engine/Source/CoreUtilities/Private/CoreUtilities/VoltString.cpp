#include "cupch.h"

#include "CoreUtilities/String/VoltString.h"

unsigned long long StoUll(const String& str, size_t* index, int32_t base /*= 10*/)
{
	int& errnoRef = errno;

	const char* ptr = str.c_str();
	char* ePtr;

	errnoRef = 0;

	const unsigned long long ans = strtoull(ptr, &ePtr, base);

	if (ptr == ePtr)
	{
		VT_ENSURE_MSG(false, "Invalid stoull argument");
	}

	if (errnoRef == ERANGE)
	{
		VT_ENSURE_MSG(false, "stoull argument out of range");
	}

	if (index)
	{
		*index = static_cast<size_t>(ePtr - ptr);
	}

	return ans;
}

int StoI(const String& str, size_t* index /*= nullptr*/, int32_t base /*= 10*/)
{
	int& errnoRef = errno;

	const char* ptr = str.c_str();
	char* ePtr;

	errnoRef = 0;

	const long ans = strtol(ptr, &ePtr, base);

	if (ptr == ePtr)
	{
		VT_ENSURE_MSG(false, "Invalid stoi argument");
	}

	if (errnoRef == ERANGE)
	{
		VT_ENSURE_MSG(false, "stoi argument out of range");
	}

	if (index)
	{
		*index = static_cast<size_t>(ePtr - ptr);
	}

	return static_cast<int>(ans);
}

float StoF(const String& str, size_t* index /*= nullptr*/)
{
	int& errnoRef = errno;

	const char* ptr = str.c_str();
	char* ePtr;

	errnoRef = 0;

	const float ans = strtof(ptr, &ePtr);

	if (ptr == ePtr)
	{
		VT_ENSURE_MSG(false, "Invalid stoi argument");
	}

	if (errnoRef == ERANGE)
	{
		VT_ENSURE_MSG(false, "stoi argument out of range");
	}

	if (index)
	{
		*index = static_cast<size_t>(ePtr - ptr);
	}

	return ans;
}
