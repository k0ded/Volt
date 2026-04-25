#pragma once
#include <cstdint>

// Note: These are special values that can be used in shaders
//		 to define that the resource is a special type.
namespace Volt::RHI::Globals
{
	inline static constexpr uint32_t SHADER_GLOBALS_BINDING = 0;
	inline static constexpr uint32_t SHADER_GLOBALS_SPACE = 0;
	inline static constexpr uint32_t SHADER_BINDLESS_SPACE = 11;
}
