#pragma once

#include "Volt-Renderer/Config.h"
#include "Volt-Renderer/Material/MaterialShader.h"

#include <CoreUtilities/TypeTraits/TypeIndex.h>
#include <CoreUtilities/Containers/Map.h>

namespace Volt
{
	class MaterialShaderRegistry
	{
	public:
		struct ShaderRegistrationInfo
		{
			std::filesystem::path baseFilepath;
			std::string entryPoint;
		};

		template<typename T>
		void RegisterShader(const std::filesystem::path& baseFilepath, const std::string& entryPoint)
		{
			static_assert(std::is_base_of_v<MaterialShader, T>);

			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();
			VT_ENSURE(!m_shaderRegistrationInfo.contains(typeIndex));

			ShaderRegistrationInfo& registrationInfo = m_shaderRegistrationInfo[typeIndex];
			registrationInfo.baseFilepath = baseFilepath;
			registrationInfo.entryPoint = entryPoint;
		}

		template<typename T>
		void UnregisterShader()
		{
			static_assert(std::is_base_of_v<MaterialShader, T>);

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

		VTR_API static MaterialShaderRegistry& Get();
	
		VT_INLINE VT_NODISCARD const Map<TypeTraits::TypeIndex, ShaderRegistrationInfo>& GetRegisteredShaders() const { return m_shaderRegistrationInfo; }

	private:
		Map<TypeTraits::TypeIndex, ShaderRegistrationInfo> m_shaderRegistrationInfo;
	};
}

// Must lie in a compilation unit (cpp file)
#define VT_REGISTER_MATERIAL_SHADER(klass, filepath, entryPoint) \
	class MaterialShaderRegistrar_##klass \
	{ \
	public: \
		VT_INLINE MaterialShaderRegistrar_##klass() \
		{ \
			Volt::MaterialShaderRegistry::Get().RegisterShader<klass>(filepath, entryPoint); \
		} \
		VT_INLINE ~MaterialShaderRegistrar_##klass() \
		{ \
			Volt::MaterialShaderRegistry::Get().UnregisterShader<klass>(); \
		} \
	} g_materialShaderRegistrar_##klass
