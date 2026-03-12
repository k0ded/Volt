#include "vkpch.h"

#include "VulkanRHIModule/Utility/PushConstantsBuilder.h"
#include "VulkanRHIModule/Common/VulkanHelpers.h"

namespace Volt::RHI
{
	VkPushConstantRange PushConstantsBuilder::BuildPushConstantRange(ArrayView<ShaderParameterMap> shaderParameterMaps) const
	{
		VkPushConstantRange range;
		range.offset = 0;
		range.size = 0;
		range.stageFlags = 0;

		for (const ShaderParameterMap& shaderParameterMap : shaderParameterMaps)
		{
			if (shaderParameterMap.HasInlineParameterBlock())
			{
				VT_ENSURE_MSG(range.size == 0, "Currently only one inline parameter block is supported!");

				range.size = shaderParameterMap.GetInlineParameterBlockSize();
				range.stageFlags |= Utility::VoltToVulkanShaderStage(shaderParameterMap.GetShaderStage());
			}
		}

		return range;
	}
}
