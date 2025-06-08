#pragma once

#include "RenderCore/Config.h"
#include "RenderCore/RenderGraph/ShaderParameterStruct.h"

#include <LogModule/LogCategory.h>

#include <CoreUtilities/TypeTraits/TypeIndex.h>
#include <CoreUtilities/Containers/Map.h>

#include <filesystem>

namespace Volt
{
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

			return true;
		}

		VT_INLINE VT_NODISCARD const vt::map<TypeTraits::TypeIndex, ShaderRegistrationInfo>& GetRegisteredShaders() const { return m_shaderRegistrationInfo; }
		VT_INLINE VT_NODISCARD const ShaderRegistrationInfo& GetShaderRegistrationInfo(const TypeTraits::TypeIndex typeIndex) const { return m_shaderRegistrationInfo.at(typeIndex); }

		static ShaderRegistry& Get();

	private:
		struct TypeIndexContainer
		{
			TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<void>();
		};

		friend class ShaderSubSystem;

		vt::map<TypeTraits::TypeIndex, ShaderRegistrationInfo> m_shaderRegistrationInfo;
 	};
}

#define REGISTER_SHADER(klass, filepath, entryPoint, shaderStage) \
	inline static bool ShaderRegistry_## klass ## _Registered = Volt::ShaderRegistry::Get().RegisterShader<klass>(filepath, entryPoint, Volt::RHI::ShaderStage::shaderStage)
	
