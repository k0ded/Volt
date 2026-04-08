#pragma once

#include "RenderCore/Config.h"
#include "RenderCore/RenderGraph/ShaderParameterStruct.h"

#include <LogModule/LogCategory.h>

#include <CoreUtilities/TypeTraits/TypeIndex.h>
#include <CoreUtilities/Containers/Map.h>

#include <filesystem>

namespace Volt
{
	template<typename T>
	concept HasshaderPermutations = requires
	{
		typename T::PermutationVector;
	};

	class VTRC_API ShaderRegistry
	{
	public:
		struct ShaderStageInfo
		{
			Filesystem::Path filePath;
			String entryPoint;
			RHI::ShaderStage shaderStage;
			bool hasPermutations;
		};

		struct ShaderRegistrationInfo
		{
			ShaderStageInfo stageInfos;
			StringView name;
		};

		template<typename T>
		void RegisterShader(const Filesystem::Path& filepath, const String& entryPoint, RHI::ShaderStage shaderStage)
		{
			static_assert(std::is_base_of_v<GlobalShader, T>);

			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();
			VT_ENSURE(!m_shaderRegistrationInfo.contains(typeIndex));

			ShaderRegistrationInfo& registrationInfo = m_shaderRegistrationInfo[typeIndex];

			registrationInfo.name = T::shaderName;
			registrationInfo.stageInfos.filePath = filepath;
			registrationInfo.stageInfos.shaderStage = shaderStage;
			registrationInfo.stageInfos.entryPoint = entryPoint;
			registrationInfo.stageInfos.hasPermutations = false;

			if constexpr (HasshaderPermutations<T>)
			{
				registrationInfo.stageInfos.hasPermutations = true;
			}
		}

		template<typename T>
		void UnregisterShader()
		{
			static_assert(std::is_base_of_v<GlobalShader, T>);

			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();

			// #Note_Ivar: The registry may already have been destroyed due to
			// DLL ordering.
			if (m_shaderRegistrationInfo.empty())
			{
				return;
			}

			if (VT_CHECK(m_shaderRegistrationInfo.contains(typeIndex)))
			{
				m_shaderRegistrationInfo.erase(typeIndex);
			}
		}

		VT_NODISCARD VT_INLINE const Map<TypeTraits::TypeIndex, ShaderRegistrationInfo>& GetRegisteredShaders() const { return m_shaderRegistrationInfo; }
		VT_NODISCARD VT_INLINE const ShaderRegistrationInfo& GetShaderRegistrationInfo(const TypeTraits::TypeIndex typeIndex) const { return m_shaderRegistrationInfo.at(typeIndex); }

		static ShaderRegistry& Get();

	private:
		struct TypeIndexContainer
		{
			TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<void>();
		};

		friend class ShaderSubSystem;

		Map<TypeTraits::TypeIndex, ShaderRegistrationInfo> m_shaderRegistrationInfo;
 	};
}

// Must lie in a compilation unit (cpp file)
#define VT_REGISTER_SHADER(klass, filepath, entryPoint, shaderStage) \
	class ShaderRegistrar_##klass \
	{ \
	public: \
		VT_INLINE ShaderRegistrar_##klass() \
		{ \
			Volt::ShaderRegistry::Get().RegisterShader<klass>(filepath, entryPoint, Volt::RHI::ShaderStage::shaderStage); \
		} \
		VT_INLINE ~ShaderRegistrar_##klass() \
		{ \
			Volt::ShaderRegistry::Get().UnregisterShader<klass>(); \
		} \
	} g_shaderRegistrar_##klass
