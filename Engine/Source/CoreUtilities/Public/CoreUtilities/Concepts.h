#pragma once

#include <type_traits>
#include <glm/glm.hpp>

template<typename T>
concept Integer = std::is_integral_v<T>;

template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template<typename T>
concept Enum = std::is_enum_v<T>;

template<typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

template<typename T, typename U>
concept IsDerivedFrom = std::is_base_of_v<U, T>;

template<typename T, typename U>
concept IsParentOf = std::is_base_of_v<T, U>;

template<typename T>
concept Pod = std::is_pod_v<T>;

template<typename T>
concept MathType = std::_Is_any_of_v<std::remove_cv_t<T>,
	glm::vec2, glm::vec3, glm::vec4, 
	glm::uvec2, glm::uvec3, glm::uvec4,
	glm::ivec2, glm::ivec3, glm::ivec4,
	glm::mat4, glm::mat3, glm::mat3x4,
	glm::quat>;
