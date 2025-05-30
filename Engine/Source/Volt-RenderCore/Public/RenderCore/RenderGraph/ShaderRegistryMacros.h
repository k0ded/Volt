#pragma once

#include "RenderCore/RenderGraph/ShaderRegistry.h"

#include <RHIModule/Shader/ShaderCommon.h>

namespace Volt
{
	template<size_t N>
	struct ConstexprShaderString
	{
		constexpr ConstexprShaderString(const char(&str)[N])
		{
			for (size_t i = 0; i < N; ++i)
			{
				data[i] = str[i];
			}
		}

		constexpr std::string_view GetView() const
		{
			return { data.data(), N - 1 };
		}

		std::array<char, N> data;
	};

	template <ConstexprShaderString File, ConstexprShaderString Entry, RHI::ShaderStage Stage>
	struct ShaderStageWrapper
	{
		static constexpr auto filePath = File;
		static constexpr auto entryPoint = Entry;
		static constexpr RHI::ShaderStage stage = Stage;
	};

	// **Variadic template struct to hold shader stages with `std::tuple`**
	template <typename... Stages>
	struct ShaderStageList
	{
		static constexpr auto stages = std::tuple{ Stages{}... };
	};
}

#define DECLARE_SHADER_STAGE(filePath, entryPoint, shaderStage) \
	, Volt::ShaderStageWrapper<Volt::ConstexprShaderString{ filePath }, Volt::ConstexprShaderString{ entryPoint }, shaderStage>

#define BEGIN_SHADER_DEFINITION(klass) \
	inline static constexpr std::string_view shaderName = #klass; \
	using ShaderStages = ShaderStageList<Volt::ShaderStageWrapper<Volt::ConstexprShaderString{ "s" }, Volt::ConstexprShaderString{ "s" }, RHI::ShaderStage::None>

#define END_SHADER_DEFINITION() \
	>; \
	static constexpr auto GetStages() { return ShaderStages::stages; } 

#define DECLARE_GLOBAL_SHADER(klass) \
	inline static constexpr std::string_view shaderName = #klass; \
