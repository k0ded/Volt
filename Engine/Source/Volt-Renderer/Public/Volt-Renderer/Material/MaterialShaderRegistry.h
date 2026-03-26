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
			Filesystem::Path baseFilepath;
			String entryPoint;
			TypeTraits::TypeIndex defaultShaderClass = TypeTraits::TypeIndex::FromType<void>();
		};

		template<typename ShaderClass, typename DefaultShaderClass>
		void RegisterShader(const Filesystem::Path& baseFilepath, const String& entryPoint)
		{
			static_assert(std::is_base_of_v<MaterialShader, ShaderClass>);

			constexpr TypeTraits::TypeIndex shaderType = TypeTraits::TypeIndex::FromType<ShaderClass>();
			constexpr TypeTraits::TypeIndex defaultShaderType = TypeTraits::TypeIndex::FromType<DefaultShaderClass>();

			VT_ENSURE(!m_shaderRegistrationInfo.contains(shaderType));

			ShaderRegistrationInfo& registrationInfo = m_shaderRegistrationInfo[shaderType];
			registrationInfo.baseFilepath = baseFilepath;
			registrationInfo.entryPoint = entryPoint;
			registrationInfo.defaultShaderClass = defaultShaderType;
		}

		template<typename ShaderClass>
		void UnregisterShader()
		{
			static_assert(std::is_base_of_v<MaterialShader, ShaderClass>);

			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<ShaderClass>();

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
		VT_INLINE VT_NODISCARD const ShaderRegistrationInfo& GetShaderRegistrationInfoForShader(TypeTraits::TypeIndex typeIndex) const { return m_shaderRegistrationInfo.at(typeIndex); }

	private:
		Map<TypeTraits::TypeIndex, ShaderRegistrationInfo> m_shaderRegistrationInfo;
	};
}

// Must lie in a compilation unit (cpp file)
#define VT_REGISTER_MATERIAL_SHADER(klass, defaultShaderClass, filepath, entryPoint) \
	class MaterialShaderRegistrar_##klass \
	{ \
	public: \
		VT_INLINE MaterialShaderRegistrar_##klass() \
		{ \
			Volt::MaterialShaderRegistry::Get().RegisterShader<klass, defaultShaderClass>(filepath, entryPoint); \
		} \
		VT_INLINE ~MaterialShaderRegistrar_##klass() \
		{ \
			Volt::MaterialShaderRegistry::Get().UnregisterShader<klass>(); \
		} \
	} g_materialShaderRegistrar_##klass
