#pragma once

#include <RHIModule/Shader/ShaderCommon.h>

#define DECLARE_GLOBAL_SHADER(klass) \
	inline static constexpr std::string_view shaderName = #klass; \
