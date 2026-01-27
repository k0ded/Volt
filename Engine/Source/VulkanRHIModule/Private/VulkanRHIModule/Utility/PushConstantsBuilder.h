#pragma once

#include <RHIModule/Shader/ShaderParameterMap.h>

#include <CoreUtilities/Containers/ArrayView.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	class PushConstantsBuilder
	{
	public:
		VkPushConstantRange BuildPushConstantRange(ArrayView<ShaderParameterMap> shaderParameterMaps) const;
	};
}
