#pragma once

#include "RenderCore/Config.h"
#include "RenderCore/RenderGraph/ShaderParameterStruct.h"

#include <RHIModule/Shader/Shader.h>

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

	struct ShaderRenderGraphConstantsData;

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
			Vector<ShaderStageInfo> stageInfos;
			Vector<ShaderParameterMetadata> parameterMetadata;
			std::string_view name;
		};

		template<typename T>
		bool RegisterShader()
		{
			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();
			VT_ENSURE(!m_shaderRegistrationInfo.contains(typeIndex));

			constexpr auto shaderStages = T::GetStages();

			ShaderRegistrationInfo& registrationInfo = m_shaderRegistrationInfo[typeIndex];

			ShaderStageIterator(shaderStages, [&](const auto& stageInfo)
			{
				// There will always be an initial entry which is empty. 
				if (stageInfo.stage != RHI::ShaderStage::None)
				{
					registrationInfo.stageInfos.emplace_back(stageInfo.filePath.GetView().data(), stageInfo.entryPoint.GetView().data(), stageInfo.stage);
				}
			});

			registrationInfo.name = T::shaderName;

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
		friend class ShaderSubSystem;

		void CorrectShaderParameterMetadataOffsets(TypeTraits::TypeIndex typeIndex, const RHI::ShaderRenderGraphConstantsData& reflectedConstants);

		vt::map<TypeTraits::TypeIndex, ShaderRegistrationInfo> m_shaderRegistrationInfo;
	};
}

extern VTRC_API Volt::ShaderRegistry g_shaderRegistry;

VT_INLINE Volt::ShaderRegistry& GetShaderRegistry()
{
	return g_shaderRegistry;
}

#define REGISTER_SHADER(klass) \
	inline static bool ShaderRegistry_## klass ## _Registered = GetShaderRegistry().RegisterShader<klass>();
	
