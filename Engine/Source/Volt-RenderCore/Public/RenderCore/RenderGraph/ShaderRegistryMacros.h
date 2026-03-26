#pragma once

#include <RHIModule/Shader/ShaderCommon.h>

#define DECLARE_GLOBAL_SHADER(klass) \
	public: \
	inline static constexpr StringView shaderName = #klass; \
