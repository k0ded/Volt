#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <locale>

namespace StringAlgorithm
{
	// Specialized char version of STL find() from back function.
	// Not the same as RFind because search range is specified as forward iterators.
	template <typename T>
	const T* StringFindEnd(const T* pBegin, const T* pEnd, T c)
	{
		const T* pTemp = pEnd;
		while (--pTemp >= pBegin)
		{
			if (*pTemp == c)
				return pTemp;
		}

		return pEnd;
	}

	template<typename T>
	inline const T* StringRFind(const T* rBegin, const T* rEnd, const T c)
	{
		while (rBegin > rEnd)
		{
			if (*(rBegin - 1) == c)
			{
				return rBegin;
			}

			--rBegin;
		}

		return rEnd;
	}

	template<typename T>
	inline const T* StringFindFirstOf(const T* p1Begin, const T* p1End, const T* p2Begin, const T* p2End)
	{
		for (; p1Begin != p1End; ++p1Begin)
		{
			for (const T* temp = p2Begin; temp != p2End; ++temp)
			{
				if (*p1Begin == *temp)
				{
					return p1Begin;
				}
			}
		}

		return p1End;
	}

	template<typename T>
	inline const T* StringRFindFirstOf(const T* p1Begin, const T* p1End, const T* p2Begin, const T* p2End)
	{
		for (; p1Begin != p1End; --p1Begin)
		{
			for (const T* temp = p2Begin; temp != p2End; ++temp)
			{
				if (*(p1Begin - 1) == *temp)
				{
					return p1Begin;
				}
			}
		}

		return p1End;
	}

	// Specialized value_type version of STL find_end() function (which really is a reverse search function).
	// Purpose: find last instance of p2 within p1. Return p1End if not found or if either string is zero length.
	template <typename T>
	const T* StringRSearch(const T* p1Begin, const T* p1End,
								   const T* p2Begin, const T* p2End)
	{
		// Test for zero length strings, in which case we have a match or a failure, 
		// but the return value is the same either way.
		if ((p1Begin == p1End) || (p2Begin == p2End))
			return p1Begin;

		// Test for a pattern of length 1.
		if ((p2Begin + 1) == p2End)
			return StringFindEnd(p1Begin, p1End, *p2Begin);

		// Test for search string length being longer than string length.
		if ((p2End - p2Begin) > (p1End - p1Begin))
			return p1End;

		// General case.
		const T* pSearchEnd = (p1End - (p2End - p2Begin) + 1);

		const T* pMatchCandidate;
		while ((pMatchCandidate = StringFindEnd(p1Begin, pSearchEnd, *p2Begin)) != pSearchEnd)
		{
			// In this case, *pMatchCandidate == *p2Begin. So compare the rest.
			const T* pCurrent1 = pMatchCandidate;
			const T* pCurrent2 = p2Begin;
			while (*pCurrent1++ == *pCurrent2++)
			{
				if (pCurrent2 == p2End)
					return (pCurrent1 - (p2End - p2Begin));
			}

			// This match failed, search again with this new end.
			pSearchEnd = pMatchCandidate;
		}

		return p1End;
	}

	template <typename T>
	inline const T* StringRFindFirstNotOf(const T* p1RBegin, const T* p1REnd, const T* p2Begin, const T* p2End)
	{
		for (; p1RBegin != p1REnd; --p1RBegin)
		{
			const T* pTemp;
			for (pTemp = p2Begin; pTemp != p2End; ++pTemp)
			{
				if (*(p1RBegin - 1) == *pTemp)
				{
					break;
				}
			}
			if (pTemp == p2End)
			{
				return p1RBegin;
			}
		}
		return p1REnd;
	}

	template<typename T>
	inline const T* StringFindFirstNotOf(const T* p1Begin, const T* p1End, const T* p2Begin, const T* p2End)
	{
		for (; p1Begin != p1End; ++p1Begin)
		{
			const T* temp;
			for (temp = p2Begin; temp != p2End; ++temp)
			{
				if (*p1Begin == *temp)
				{
					break;
				}
			}

			if (temp == p2End)
			{
				return p1Begin;
			}
		}

		return p1End;
	}

	constexpr size_t Strlen(const char* p) { return __builtin_strlen(p); }
	constexpr size_t Strlen(const wchar_t* p) { return __builtin_wcslen(p); }
	constexpr int Compare(const char* p1, const char* p2, size_t n) { return __builtin_memcmp(p1, p2, n); }
	constexpr int Compare(const wchar_t* p1, const wchar_t* p2, size_t n) { return __builtin_wmemcmp(p1, p2, n); }

	template <typename T>
	inline int CompareI(const T* p1, const T* p2, size_t n)
	{
		for (; n > 0; ++p1, ++p2, --n)
		{
			const T c1 = std::tolower(*p1);
			const T c2 = std::tolower(*p2);

			if (c1 != c2)
				return (static_cast<typename std::make_unsigned<T>::type>(c1) <
						static_cast<typename std::make_unsigned<T>::type>(c2)) ? -1 : 1;
		}
		return 0;
	}

	template<typename T>
	inline T* StringUninitializedCopy(const T* source, const T* sourceEnd, T* destination)
	{
		memmove(destination, source, (size_t)(sourceEnd - source) * sizeof(T));
		return destination + (sourceEnd - source);
	}

	inline char* StringUninitializedFillN(char* destination, size_t n, const char c)
	{
		if (n > 0)
		{
			memset(destination, static_cast<uint8_t>(c), static_cast<size_t>(n));
		}

		return destination + n;
	}

	template<typename T>
	inline T* StringUninitializedFillN(T* destination, size_t n, const T c)
	{
		T* dest = destination;
		const T* const end = destination + n;

		while (dest < end)
		{
			*dest++ = c;
		}

		return destination + n;
	}

	inline char* AssignN(char* pDestination, size_t n, char c)
	{
		return (char*)memset(pDestination, c, (size_t)n);
	}

	template<typename T>
	inline T* AssignN(T* pDestination, size_t n, T c)
	{
		T* pDest = pDestination;
		const T* const pEnd = pDestination + n;
		while (pDest < pEnd)
			*pDest++ = c;
		return pDestination;
	}
}
