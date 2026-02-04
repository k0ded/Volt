#include "rcpch.h"

#include "RenderCore/RenderGraph/ShaderParameterStruct.h"

namespace Volt
{
	ShaderParameterMetadataDescription::ShaderParameterMetadataDescription(Vector<ShaderParameterMetadata>&& shaderParameterMetadata)
		: m_metadata(std::move(shaderParameterMetadata))
	{
	}
}
