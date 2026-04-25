#include "rhipch.h"
#include "RHIModule/Shader/ShaderCommon.h"

namespace Volt::RHI
{
	ShaderUniform::ShaderUniform(const ShaderUniformType type, const size_t size, const size_t offset)
		: type(type), size(size), offset(offset)
	{
	}

	Archive& operator<<(Archive& archive, ShaderResourceBinding& value)
	{
		archive << value.set;
		archive << value.binding;
		archive << value.arraySize;
		archive << value.registerType;
		archive << value.resourceType;
		archive << value.shaderStage;
		archive << value.name;
		archive << value.isBindless;
		archive << value.bindlessHash;

		return archive;
	}
}

