#pragma once

#include "CoreUtilities/String/VoltString.h"

#include <format>

template<int = 0>
String VFormatString(const std::string_view format, const std::format_args args)
{
	String result;
	result.reserve(format.size() + args._Estimate_required_capacity());
	std::vformat_to(std::back_insert_iterator{ result }, format, args);
	return result;
}

template<int = 0>
WString VFormatString(const std::wstring_view format, const std::wformat_args args)
{
	WString result;
	result.reserve(format.size() + args._Estimate_required_capacity());
	std::vformat_to(std::back_insert_iterator{ result }, format, args);
	return result;
}

template<typename... Types>
String FormatString(const std::format_string<Types...> fmt, Types&&... args)
{
	return VFormatString(fmt.get(), std::make_format_args(args...));
}

template<typename... Types>
WString FormatString(const std::wformat_string<Types...> fmt, Types&&... args)
{
	return VFormatString(fmt.get(), std::make_wformat_args(args...));
}
