#pragma once

#include <type_traits>

template<typename T>
concept Integer = std::is_integral_v<T>;

template<typename T>
concept Enum = std::is_enum_v<T>;

template<typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

template<typename T, typename U>
concept IsDerivedFrom = std::is_base_of_v<U, T>;

template<typename T, typename U>
concept IsParentOf = std::is_base_of_v<T, U>;
