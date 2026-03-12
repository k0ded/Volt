#pragma once

#include <tuple>

namespace Utility
{
	template<typename T, typename Tuple>
	struct TupleTypeIndexHelper;

	template<typename T>
	struct TupleTypeIndexHelper<T, std::tuple<>>
	{
		static constexpr std::size_t Value = static_cast<size_t>(-1);
		static constexpr bool IsValid = false;
	};

	template<typename T, typename... Types>
	struct TupleTypeIndexHelper<T, std::tuple<T, Types...>>
	{
		static constexpr std::size_t Value = 0;
		static constexpr bool IsValid = false;
	};

	template<typename T, typename U, typename... Types>
	struct TupleTypeIndexHelper<T, std::tuple<U, Types...>>
	{
		static constexpr std::size_t Value = 1 + TupleTypeIndexHelper<T, std::tuple<Types...>>::Value;
		static constexpr bool IsValid = false;
	};

	template<typename T, typename Tuple>
	struct TupleTypeIndex
	{
		static constexpr std::size_t Value = TupleTypeIndexHelper<T, Tuple>::Value;
		static constexpr bool IsValid = Value < std::tuple_size_v<Tuple>;
	};
}
