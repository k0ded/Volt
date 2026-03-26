#include "cupch.h"

#include "CoreUtilities/String/StringBuilder.h"
#include "CoreUtilities/String/StringFormat.h"

void StringBuilder::Reserve(size_t size)
{
	m_string.reserve(size);
}

void StringBuilder::Clear()
{
	m_string.clear();
}

StringBuilder& StringBuilder::operator<<(bool value)
{
	m_string += FormatString("{}", value);
	return *this;
}

StringBuilder& StringBuilder::operator<<(uint8_t value)
{
	m_string += FormatString("{}", value);
	return *this;
}

StringBuilder& StringBuilder::operator<<(int8_t value)
{
	m_string += FormatString("{}", value);
	return *this;
}

StringBuilder& StringBuilder::operator<<(uint16_t value)
{
	m_string += FormatString("{}", value);
	return *this;
}

StringBuilder& StringBuilder::operator<<(int16_t value)
{
	m_string += FormatString("{}", value);
	return *this;
}

StringBuilder& StringBuilder::operator<<(uint32_t value)
{
	m_string += FormatString("{}", value);
	return *this;
}

StringBuilder& StringBuilder::operator<<(int32_t value)
{
	m_string += FormatString("{}", value);
	return *this;
}

StringBuilder& StringBuilder::operator<<(uint64_t value)
{
	m_string += FormatString("{}", value);
	return *this;
}

StringBuilder& StringBuilder::operator<<(int64_t value)
{
	m_string += FormatString("{}", value);
	return *this;
}

StringBuilder& StringBuilder::operator<<(float value)
{
	m_string += FormatString("{}", value);
	return *this;
}

StringBuilder& StringBuilder::operator<<(double value)
{
	m_string += FormatString("{}", value);
	return *this;
}

StringBuilder& StringBuilder::operator<<(const String& value)
{
	m_string += value;
	return *this;
}

StringBuilder& StringBuilder::operator<<(const StringView value)
{
	m_string += String(value);
	return *this;
}
