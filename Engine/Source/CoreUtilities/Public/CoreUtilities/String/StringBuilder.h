#pragma once

#include "CoreUtilities/Config.h"
#include "CoreUtilities/String/VoltString.h"

class StringBuilder
{
public:
	StringBuilder() = default;
	~StringBuilder() = default;

	VTCOREUTIL_API void Reserve(size_t size);
	VTCOREUTIL_API void Clear();

	VT_NODISCARD VT_INLINE const String& Get() const { return m_string; }

	VTCOREUTIL_API StringBuilder& operator<<(bool value);
	
	VTCOREUTIL_API StringBuilder& operator<<(uint8_t value);
	VTCOREUTIL_API StringBuilder& operator<<(int8_t value);

	VTCOREUTIL_API StringBuilder& operator<<(uint16_t value);
	VTCOREUTIL_API StringBuilder& operator<<(int16_t value);

	VTCOREUTIL_API StringBuilder& operator<<(uint32_t value);
	VTCOREUTIL_API StringBuilder& operator<<(int32_t value);

	VTCOREUTIL_API StringBuilder& operator<<(uint64_t value);
	VTCOREUTIL_API StringBuilder& operator<<(int64_t value);

	VTCOREUTIL_API StringBuilder& operator<<(float value);
	VTCOREUTIL_API StringBuilder& operator<<(double value);

	VTCOREUTIL_API StringBuilder& operator<<(const String& value);
	VTCOREUTIL_API StringBuilder& operator<<(const StringView value);

	VTCOREUTIL_API StringBuilder& operator<<(const char* str);
	VTCOREUTIL_API StringBuilder& operator<<(const wchar_t* wstr);

private:
	String m_string;
};
