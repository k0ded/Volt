#include "rcpch.h"

#include "RenderCore/RenderGraph/ShaderRegistry.h"

#include <RHIModule/Shader/ShaderCommon.h>

Volt::ShaderRegistry g_shaderRegistry;

namespace Volt
{
	void ShaderRegistry::CorrectShaderParameterMetadataOffsets(TypeTraits::TypeIndex typeIndex, const RHI::ShaderUniforms& reflectedConstants)
	{
		auto& registrationInfo = m_shaderRegistrationInfo.at(typeIndex);

		for (auto& metadata : registrationInfo.parameterMetadata)
		{
			if (reflectedConstants.uniforms.contains(metadata.hashedName))
			{
				auto& uniform = reflectedConstants.uniforms.at(metadata.hashedName);
				metadata.reflectedOffset = static_cast<uint32_t>(uniform.offset);
			}
			else
			{
				metadata.reflectedOffset = std::numeric_limits<uint32_t>::max();
			}
		}
	}
}
