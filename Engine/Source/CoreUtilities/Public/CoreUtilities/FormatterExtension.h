#pragma once

#include <glm/glm.hpp>

#include <format>
#include <iostream>

template<>
struct std::formatter<glm::vec2> : std::formatter<std::string>
{
	template<class FmtContext>
	FmtContext::iterator format(glm::vec2 s, FmtContext& ctx) const
	{
		return std::format_to(ctx.out(), "float2({0}, {1})", s.x, s.y);
	}
};

template<>
struct std::formatter<glm::vec3> : std::formatter<std::string>
{
	template<class FmtContext>
	FmtContext::iterator format(glm::vec3 s, FmtContext& ctx) const
	{
		return std::format_to(ctx.out(), "float3({0}, {1}, {2})", s.x, s.y, s.z);
	}
};

template<>
struct std::formatter<glm::vec4> : std::formatter<std::string>
{
	template<class FmtContext>
	FmtContext::iterator format(glm::vec4 s, FmtContext& ctx) const
	{
		return std::format_to(ctx.out(), "float4({0}, {1}, {2}, {3})", s.x, s.y, s.z, s.w);
	}
};

template<>
struct std::formatter<glm::quat> : std::formatter<std::string>
{
	template<class FmtContext>
	FmtContext::iterator format(glm::quat s, FmtContext& ctx) const
	{
		return std::format_to(ctx.out(), "quat({0}, {1}, {2}, {3})", s.x, s.y, s.z, s.w);
	}
};

template<>
struct std::formatter<glm::ivec2> : std::formatter<std::string>
{
	template<class FmtContext>
	FmtContext::iterator format(glm::ivec2 s, FmtContext& ctx) const
	{
		return std::format_to(ctx.out(), "int2({0}, {1})", s.x, s.y);
	}
};

template<>
struct std::formatter<glm::ivec3> : std::formatter<std::string>
{
	template<class FmtContext>
	FmtContext::iterator format(glm::ivec3 s, FmtContext& ctx) const
	{
		return std::format_to(ctx.out(), "int3({0}, {1}, {2})", s.x, s.y, s.z);
	}
};

template<>
struct std::formatter<glm::ivec4> : std::formatter<std::string>
{
	template<class FmtContext>
	FmtContext::iterator format(glm::ivec4 s, FmtContext& ctx) const
	{
		return std::format_to(ctx.out(), "int4({0}, {1}, {2}, {3})", s.x, s.y, s.z, s.w);
	}
};
