#pragma once

#include "RenderCore/Config.h"
#include "RenderCore/RenderGraph2/ShaderParameterStruct2.h"
#include "RenderCore/Shader/GlobalShader.h"

#include <LogModule/LogCategory.h>

#include <CoreUtilities/TypeTraits/TypeIndex.h>
#include <CoreUtilities/Containers/Map.h>

#include <filesystem>

namespace Volt
{
	template<typename T>
	concept HasSetupParametersFunc = requires
	{
		{ T::zzInternal_ProcessMembers };
	};

	template<typename T>
	concept HasParametersStruct = requires { typename T::Parameters; };

	template <std::size_t Index = 0, typename Func, typename... Stages>
	constexpr void ShaderStageIterator(const std::tuple<Stages...>& stages, Func&& func)
	{
		if constexpr (Index < sizeof...(Stages))
		{
			func(std::get<Index>(stages));
			ShaderStageIterator<Index + 1>(stages, std::forward<Func>(func));
		}
	}

	struct ShaderUniforms;

	class VTRC_API ShaderRegistry
	{
	public:
		struct ShaderStageInfo
		{
			std::filesystem::path filePath;
			std::string entryPoint;
			RHI::ShaderStage shaderStage;
		};

		struct ShaderRegistrationInfo
		{
			ShaderStageInfo stageInfos;
			Vector<ShaderParameterMetadata> parameterMetadata;
			std::string_view name;
		};

		template<typename T>
		bool RegisterShader(const std::filesystem::path& filepath, const std::string& entryPoint, RHI::ShaderStage shaderStage)
		{
			static_assert(std::is_base_of_v<GlobalShader, T>);

			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();
			VT_ENSURE(!m_shaderRegistrationInfo.contains(typeIndex));

			ShaderRegistrationInfo& registrationInfo = m_shaderRegistrationInfo[typeIndex];

			registrationInfo.name = T::shaderName;
			registrationInfo.stageInfos.filePath = filepath;
			registrationInfo.stageInfos.shaderStage = shaderStage;
			registrationInfo.stageInfos.entryPoint = entryPoint;

			if constexpr (HasParametersStruct<T>)
			{
				if constexpr (HasSetupParametersFunc<typename T::Parameters>)
				{
					T::Parameters::zzInternal_ProcessMembers(registrationInfo.parameterMetadata);
				}
			}

			return true;
		}

		VT_INLINE VT_NODISCARD const vt::map<TypeTraits::TypeIndex, ShaderRegistrationInfo>& GetRegisteredShaders() const { return m_shaderRegistrationInfo; }
		VT_INLINE VT_NODISCARD const ShaderRegistrationInfo& GetShaderRegistrationInfo(const TypeTraits::TypeIndex typeIndex) const { return m_shaderRegistrationInfo.at(typeIndex); }

	private:
		struct TypeIndexContainer
		{
			TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<void>();
		};

		friend class ShaderSubSystem;

		vt::map<TypeTraits::TypeIndex, ShaderRegistrationInfo> m_shaderRegistrationInfo;
 	};
}

extern VTRC_API Volt::ShaderRegistry g_shaderRegistry;

VT_INLINE Volt::ShaderRegistry& GetShaderRegistry()
{
	return g_shaderRegistry;
}

#define REGISTER_SHADER(klass, filepath, entryPoint, shaderStage) \
	inline static bool ShaderRegistry_## klass ## _Registered = GetShaderRegistry().RegisterShader<klass>(filepath, entryPoint, Volt::RHI::ShaderStage::shaderStage)
	
