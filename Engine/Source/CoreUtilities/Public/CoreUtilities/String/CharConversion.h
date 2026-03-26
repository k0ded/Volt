#pragma once

#include "CoreUtilities/Config.h"

///////////////////////////////////////////////////////////////////////////////
/// DecodePart
///
/// These implement UTF8/UCS2/UCS4 encoding/decoding.
///

inline static constexpr size_t SizeOfWChar = sizeof(wchar_t);

VTCOREUTIL_API bool DecodePart(const char*& pSrc, const char* pSrcEnd, char*& pDest, char* pDestEnd);
VTCOREUTIL_API bool DecodePart(const char*& pSrc, const char* pSrcEnd, char16_t*& pDest, char16_t* pDestEnd);
VTCOREUTIL_API bool DecodePart(const char*& pSrc, const char* pSrcEnd, char32_t*& pDest, char32_t* pDestEnd);

VTCOREUTIL_API bool DecodePart(const char16_t*& pSrc, const char16_t* pSrcEnd, char*& pDest, char* pDestEnd);
VTCOREUTIL_API bool DecodePart(const char16_t*& pSrc, const char16_t* pSrcEnd, char16_t*& pDest, char16_t* pDestEnd);
VTCOREUTIL_API bool DecodePart(const char16_t*& pSrc, const char16_t* pSrcEnd, char32_t*& pDest, char32_t* pDestEnd);

VTCOREUTIL_API bool DecodePart(const char32_t*& pSrc, const char32_t* pSrcEnd, char*& pDest, char* pDestEnd);
VTCOREUTIL_API bool DecodePart(const char32_t*& pSrc, const char32_t* pSrcEnd, char16_t*& pDest, char16_t* pDestEnd);
VTCOREUTIL_API bool DecodePart(const char32_t*& pSrc, const char32_t* pSrcEnd, char32_t*& pDest, char32_t* pDestEnd);

VTCOREUTIL_API bool DecodePart(const int*& pSrc, const int* pSrcEnd, char*& pDest, char* pDestEnd);
VTCOREUTIL_API bool DecodePart(const int*& pSrc, const int* pSrcEnd, char16_t*& pDest, char16_t* pDestEnd);
VTCOREUTIL_API bool DecodePart(const int*& pSrc, const int* pSrcEnd, char32_t*& pDest, char32_t* pDestEnd);

bool DecodePart(const char8_t*& pSrc, const char8_t* pSrcEnd, char8_t*& pDest, char8_t* pDestEnd);

bool DecodePart(const char8_t*& pSrc, const char8_t* pSrcEnd, char*& pDest, char* pDestEnd);
bool DecodePart(const char8_t*& pSrc, const char8_t* pSrcEnd, char16_t*& pDest, char16_t* pDestEnd);
bool DecodePart(const char8_t*& pSrc, const char8_t* pSrcEnd, char32_t*& pDest, char32_t* pDestEnd);

bool DecodePart(const char*& pSrc, const char* pSrcEnd, char8_t*& pDest, char8_t* pDestEnd);
bool DecodePart(const char16_t*& pSrc, const char16_t* pSrcEnd, char8_t*& pDest, char8_t* pDestEnd);
bool DecodePart(const char32_t*& pSrc, const char32_t* pSrcEnd, char8_t*& pDest, char8_t* pDestEnd);

bool DecodePart(const wchar_t*& pSrc, const wchar_t* pSrcEnd, wchar_t*& pDest, wchar_t* pDestEnd);

bool DecodePart(const wchar_t*& pSrc, const wchar_t* pSrcEnd, char*& pDest, char* pDestEnd);
bool DecodePart(const wchar_t*& pSrc, const wchar_t* pSrcEnd, char16_t*& pDest, char16_t* pDestEnd);
bool DecodePart(const wchar_t*& pSrc, const wchar_t* pSrcEnd, char32_t*& pDest, char32_t* pDestEnd);

bool DecodePart(const char*& pSrc, const char* pSrcEnd, wchar_t*& pDest, wchar_t* pDestEnd);
bool DecodePart(const char16_t*& pSrc, const char16_t* pSrcEnd, wchar_t*& pDest, wchar_t* pDestEnd);
bool DecodePart(const char32_t*& pSrc, const char32_t* pSrcEnd, wchar_t*& pDest, wchar_t* pDestEnd);

bool DecodePart(const char8_t*& pSrc, const char8_t* pSrcEnd, wchar_t*& pDest, wchar_t* pDestEnd);
bool DecodePart(const wchar_t*& pSrc, const wchar_t* pSrcEnd, char8_t*& pDest, char8_t* pDestEnd);

inline bool DecodePart(const wchar_t*& pSrc, const wchar_t* pSrcEnd, wchar_t*& pDest, wchar_t* pDestEnd)
{
	return DecodePart(reinterpret_cast<const char*&>(pSrc), reinterpret_cast<const char*>(pSrcEnd), reinterpret_cast<char*&>(pDest), reinterpret_cast<char*&>(pDestEnd));
}

inline bool DecodePart(const wchar_t*& pSrc, const wchar_t* pSrcEnd, char*& pDest, char* pDestEnd)
{
	if constexpr (SizeOfWChar == 2)
	{
		return DecodePart(reinterpret_cast<const char16_t*&>(pSrc), reinterpret_cast<const char16_t*>(pSrcEnd), pDest, pDestEnd);
	}
	else if constexpr (SizeOfWChar == 4)
	{
		return DecodePart(reinterpret_cast<const char32_t*&>(pSrc), reinterpret_cast<const char32_t*>(pSrcEnd), pDest, pDestEnd);
	}
}

inline bool DecodePart(const wchar_t*& pSrc, const wchar_t* pSrcEnd, char16_t*& pDest, char16_t* pDestEnd)
{
	if constexpr (SizeOfWChar == 2)
	{
		return DecodePart(reinterpret_cast<const char16_t*&>(pSrc), reinterpret_cast<const char16_t*>(pSrcEnd), pDest, pDestEnd);
	}
	else if constexpr (SizeOfWChar == 4)
	{
		return DecodePart(reinterpret_cast<const char32_t*&>(pSrc), reinterpret_cast<const char32_t*>(pSrcEnd), pDest, pDestEnd);
	}
}

inline bool DecodePart(const wchar_t*& pSrc, const wchar_t* pSrcEnd, char32_t*& pDest, char32_t* pDestEnd)
{
	if constexpr (SizeOfWChar == 2)
	{
		return DecodePart(reinterpret_cast<const char16_t*&>(pSrc), reinterpret_cast<const char16_t*>(pSrcEnd), pDest, pDestEnd);
	}
	else if constexpr (SizeOfWChar == 4)
	{
		return DecodePart(reinterpret_cast<const char32_t*&>(pSrc), reinterpret_cast<const char32_t*>(pSrcEnd), pDest, pDestEnd);
	}
}

inline bool DecodePart(const char*& pSrc, const char* pSrcEnd, wchar_t*& pDest, wchar_t* pDestEnd)
{
	if constexpr (SizeOfWChar == 2)
	{
		return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char16_t*&>(pDest), reinterpret_cast<char16_t*>(pDestEnd));
	}
	else if constexpr (SizeOfWChar == 4)
	{
		return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char32_t*&>(pDest), reinterpret_cast<char32_t*>(pDestEnd));
	}
}

inline bool DecodePart(const char16_t*& pSrc, const char16_t* pSrcEnd, wchar_t*& pDest, wchar_t* pDestEnd)
{
	if constexpr (SizeOfWChar == 2)
	{
		return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char16_t*&>(pDest), reinterpret_cast<char16_t*>(pDestEnd));
	}
	else if constexpr (SizeOfWChar == 4)
	{
		return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char32_t*&>(pDest), reinterpret_cast<char32_t*>(pDestEnd));
	}
}

inline bool DecodePart(const char32_t*& pSrc, const char32_t* pSrcEnd, wchar_t*& pDest, wchar_t* pDestEnd)
{
	if constexpr (SizeOfWChar == 2)
	{
		return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char16_t*&>(pDest), reinterpret_cast<char16_t*>(pDestEnd));
	}
	else if constexpr (SizeOfWChar == 4)
	{
		return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char32_t*&>(pDest), reinterpret_cast<char32_t*>(pDestEnd));
	}
}

inline bool DecodePart(const char8_t*& pSrc, const char8_t* pSrcEnd, char8_t*& pDest, char8_t* pDestEnd)
{
	return DecodePart(reinterpret_cast<const char*&>(pSrc), reinterpret_cast<const char*>(pSrcEnd), reinterpret_cast<char*&>(pDest), reinterpret_cast<char*&>(pDestEnd));
}

inline bool DecodePart(const char8_t*& pSrc, const char8_t* pSrcEnd, char*& pDest, char* pDestEnd)
{
	return DecodePart(reinterpret_cast<const char*&>(pSrc), reinterpret_cast<const char*>(pSrcEnd), pDest, pDestEnd);
}

inline bool DecodePart(const char8_t*& pSrc, const char8_t* pSrcEnd, char16_t*& pDest, char16_t* pDestEnd)
{
	return DecodePart(reinterpret_cast<const char*&>(pSrc), reinterpret_cast<const char*>(pSrcEnd), pDest, pDestEnd);
}

inline bool DecodePart(const char8_t*& pSrc, const char8_t* pSrcEnd, char32_t*& pDest, char32_t* pDestEnd)
{
	return DecodePart(reinterpret_cast<const char*&>(pSrc), reinterpret_cast<const char*>(pSrcEnd), pDest, pDestEnd);
}

inline bool DecodePart(const char*& pSrc, const char* pSrcEnd, char8_t*& pDest, char8_t* pDestEnd)
{
	return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char*&>(pDest), reinterpret_cast<char*&>(pDestEnd));
}

inline bool DecodePart(const char16_t*& pSrc, const char16_t* pSrcEnd, char8_t*& pDest, char8_t* pDestEnd)
{
	return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char*&>(pDest), reinterpret_cast<char*&>(pDestEnd));
}

inline bool DecodePart(const char32_t*& pSrc, const char32_t* pSrcEnd, char8_t*& pDest, char8_t* pDestEnd)
{
	return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char*&>(pDest), reinterpret_cast<char*&>(pDestEnd));
}

inline bool DecodePart(const char8_t*& pSrc, const char8_t* pSrcEnd, wchar_t*& pDest, wchar_t* pDestEnd)
{
	if constexpr (SizeOfWChar == 2)
	{
		return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char16_t*&>(pDest), reinterpret_cast<char16_t*>(pDestEnd));
	}
	else if constexpr (SizeOfWChar == 4)
	{
		return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char32_t*&>(pDest), reinterpret_cast<char32_t*>(pDestEnd));
	}
}

inline bool DecodePart(const wchar_t*& pSrc, const wchar_t* pSrcEnd, char8_t*& pDest, char8_t* pDestEnd)
{
	if constexpr (SizeOfWChar == 2)
	{
		return DecodePart(reinterpret_cast<const char16_t*&>(pSrc), reinterpret_cast<const char16_t*>(pSrcEnd), reinterpret_cast<char*&>(pDest), reinterpret_cast<char*>(pDestEnd));
	}
	else if constexpr (SizeOfWChar == 4)
	{
		return DecodePart(reinterpret_cast<const char32_t*&>(pSrc), reinterpret_cast<const char32_t*>(pSrcEnd), reinterpret_cast<char*&>(pDest), reinterpret_cast<char*>(pDestEnd));
	}
}
