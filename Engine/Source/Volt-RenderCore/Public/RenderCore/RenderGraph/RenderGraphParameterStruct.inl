#pragma once

namespace Volt
{
	template<typename T>
	T RenderGraphParameterDesc::GetAs()
	{
		return *reinterpret_cast<std::remove_reference_t<T>*>(m_dataPtr);
	}

	template<typename FuncType>
	void RenderGraphParameterStruct::EnumerateParameters(FuncType&& func)
	{
		const Vector<ShaderParameterMetadata>& shaderParameterMetadatas = m_shaderParameterMetadata->GetParameterMetadata();

		for (const ShaderParameterMetadata& parameterMetadata : shaderParameterMetadatas)
		{
			func(RenderGraphParameterDesc(
				const_cast<uint8_t*>(m_shaderParameters) + parameterMetadata.structOffset, 
				parameterMetadata.hashedName,
				parameterMetadata.structSize,
				parameterMetadata.parameterType,
				parameterMetadata.resourceAccessType
			));
		}
	}
}
