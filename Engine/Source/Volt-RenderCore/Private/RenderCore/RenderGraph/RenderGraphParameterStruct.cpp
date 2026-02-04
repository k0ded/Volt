#include "rcpch.h"

#include "RenderCore/RenderGraph/RenderGraphParameterStruct.h"

namespace Volt
{
	RenderGraphParameterDesc::RenderGraphParameterDesc(
		uint8_t* dataPtr, 
		StringHash parameterNameHash, 
		size_t size,
		ShaderParameterType shaderParameterType, 
		RGResourceAccess resourceAccessType
	)
		: m_dataPtr(dataPtr),
		m_parameterNameHash(parameterNameHash),
		m_size(size),
		m_shaderParameterType(shaderParameterType),
		m_resourceAccessType(resourceAccessType)
	{

	}
}
