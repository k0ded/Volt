#pragma once

#include <glm/glm.hpp>

namespace vt
{
	using RawByteBuffer = void;
	using RWRawByteBuffer = void;
	using TextureSampler = void;

	template<typename T> using TypedBuffer = void;
	template<typename T> using RWTypedBuffer = void;
	template<typename T> using UniformBuffer = void;

	template<typename T> using Tex2D = void;
	template<typename T> using RWTex2D = void;
	template<typename T> using TexCube = void;
	template<typename T> using Tex2DArray = void;
	template<typename T> using RWTex2DArray = void;
	template<typename T> using Tex3D = void;
	template<typename T> using RWTex3D = void;
}

using uint = uint32_t;
using uint2 = glm::uvec2;
using uint3 = glm::uvec3;
using uint4 = glm::uvec4;

using int2 = glm::ivec2;
using int3 = glm::ivec3;
using int4 = glm::ivec4;

using float2 = glm::vec2;
using float3 = glm::vec3;
using float4 = glm::vec4;

using float4x4 = glm::mat4x4;
