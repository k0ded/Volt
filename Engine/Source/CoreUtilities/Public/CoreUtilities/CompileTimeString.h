#pragma once

#include <cstdint>

template<size_t N>
struct CompileTimeString
{
	constexpr CompileTimeString(const char(&str)[N])
	{
		for (size_t i = 0; i < N; ++i)
		{
			value[i] = str[i];
		}
	}

	constexpr operator std::string_view() const
	{
		return { value, N - 1 };
	}

	char value[N];
};

template<size_t N>
CompileTimeString(const char(&)[N]) -> CompileTimeString<N>;
