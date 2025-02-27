#pragma once

#include <vulkan/vulkan.h>

#define VT_GET_VULKAN_FUNCTION(functionName) functionName = (PFN_ ## functionName)vkGetInstanceProcAddr(instance, #functionName)

namespace Volt::RHI
{
	inline PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
	inline PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT;
	inline PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT;
	
	// Descriptor Buffers
	inline PFN_vkGetDescriptorSetLayoutSizeEXT vkGetDescriptorSetLayoutSizeEXT;
	inline PFN_vkGetDescriptorEXT vkGetDescriptorEXT;
	inline PFN_vkCmdBindDescriptorBuffersEXT vkCmdBindDescriptorBuffersEXT;
	inline PFN_vkCmdSetDescriptorBufferOffsetsEXT vkCmdSetDescriptorBufferOffsetsEXT;
	inline PFN_vkGetDescriptorSetLayoutBindingOffsetEXT vkGetDescriptorSetLayoutBindingOffsetEXT;

	// Mesh shaders
	inline PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT;
	inline PFN_vkCmdDrawMeshTasksIndirectEXT vkCmdDrawMeshTasksIndirectEXT;
	inline PFN_vkCmdDrawMeshTasksIndirectCountEXT vkCmdDrawMeshTasksIndirectCountEXT;

	// Ray tracing
	inline PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
	inline PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
	inline PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
	inline PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
	inline PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
	inline PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
	inline PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
	inline PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;

	inline static void FindVulkanFunctions(VkInstance instance)
	{
		VT_GET_VULKAN_FUNCTION(vkSetDebugUtilsObjectNameEXT);
		VT_GET_VULKAN_FUNCTION(vkCmdBeginDebugUtilsLabelEXT);
		VT_GET_VULKAN_FUNCTION(vkCmdEndDebugUtilsLabelEXT);

		// Descriptor Buffers
		VT_GET_VULKAN_FUNCTION(vkGetDescriptorSetLayoutSizeEXT);
		VT_GET_VULKAN_FUNCTION(vkGetDescriptorSetLayoutBindingOffsetEXT);
		VT_GET_VULKAN_FUNCTION(vkGetDescriptorEXT);
		VT_GET_VULKAN_FUNCTION(vkCmdBindDescriptorBuffersEXT);
		VT_GET_VULKAN_FUNCTION(vkCmdSetDescriptorBufferOffsetsEXT);

		// Mesh shaders
		VT_GET_VULKAN_FUNCTION(vkCmdDrawMeshTasksEXT);
		VT_GET_VULKAN_FUNCTION(vkCmdDrawMeshTasksIndirectEXT);
		VT_GET_VULKAN_FUNCTION(vkCmdDrawMeshTasksIndirectCountEXT);
	
		// Ray tracing
		VT_GET_VULKAN_FUNCTION(vkCmdBuildAccelerationStructuresKHR);
		VT_GET_VULKAN_FUNCTION(vkGetAccelerationStructureBuildSizesKHR);
		VT_GET_VULKAN_FUNCTION(vkCreateAccelerationStructureKHR);
		VT_GET_VULKAN_FUNCTION(vkGetAccelerationStructureDeviceAddressKHR);
		VT_GET_VULKAN_FUNCTION(vkDestroyAccelerationStructureKHR);
		VT_GET_VULKAN_FUNCTION(vkCreateRayTracingPipelinesKHR);
		VT_GET_VULKAN_FUNCTION(vkGetRayTracingShaderGroupHandlesKHR);
		VT_GET_VULKAN_FUNCTION(vkCmdTraceRaysKHR);
	}
}
