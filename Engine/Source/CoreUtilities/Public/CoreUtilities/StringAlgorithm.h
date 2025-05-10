#pragma once

namespace StringAlgorithm
{
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
}
