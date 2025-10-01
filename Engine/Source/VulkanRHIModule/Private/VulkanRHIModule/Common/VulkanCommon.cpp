#include "vkpch.h"
#include "VulkanCommon.h"

#include "VulkanRHIModule/Common/VulkanFunctions.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/GraphicsDevice.h>

#include <vulkan/vulkan.h>

const char* VKResultToString(int32_t result)
{
	switch (result)
	{
		case VK_SUCCESS: return "VK_SUCCESS";
		case VK_NOT_READY: return "VK_NOT_READY";
		case VK_TIMEOUT: return "VK_TIMEOUT";
		case VK_EVENT_SET: return "VK_EVENT_SET";
		case VK_EVENT_RESET: return "VK_EVENT_RESET";
		case VK_INCOMPLETE: return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
		case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
		case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
		case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
		case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
		case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		case VK_ERROR_NOT_PERMITTED_EXT: return "VK_ERROR_NOT_PERMITTED_EXT";
		case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
		case VK_THREAD_IDLE_KHR: return "VK_THREAD_IDLE_KHR";
		case VK_THREAD_DONE_KHR: return "VK_THREAD_DONE_KHR";
		case VK_OPERATION_DEFERRED_KHR: return "VK_OPERATION_DEFERRED_KHR";
		case VK_OPERATION_NOT_DEFERRED_KHR: return "VK_OPERATION_NOT_DEFERRED_KHR";
		case VK_PIPELINE_COMPILE_REQUIRED_EXT: return "VK_PIPELINE_COMPILE_REQUIRED_EXT";
	}
	return nullptr;
}

const char* GetAddressTypeStr(VkDeviceFaultAddressTypeEXT addressType)
{
	switch (addressType)
	{
		case VK_DEVICE_FAULT_ADDRESS_TYPE_NONE_EXT: return "None"; break;
		case VK_DEVICE_FAULT_ADDRESS_TYPE_READ_INVALID_EXT: return "ReadInvalid"; break;
		case VK_DEVICE_FAULT_ADDRESS_TYPE_WRITE_INVALID_EXT: return "WriteInvalid"; break;
		case VK_DEVICE_FAULT_ADDRESS_TYPE_EXECUTE_INVALID_EXT: return "ExecuteInvalid"; break;
		case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_UNKNOWN_EXT: return "InstructionPointerUnknown"; break;
		case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_INVALID_EXT: return "InstructionPointerInvalid"; break;
		case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_FAULT_EXT: return "InstructionPointerFault"; break;
	}

	return "Invalid";
}

void HandleDeviceLost()
{
#if 0
	VkDeviceFaultCountsEXT counts{};
	counts.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT;

	VkDeviceFaultInfoEXT info{};
	info.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT;
	info.pNext = nullptr;
	info.pAddressInfos = nullptr;
	info.pVendorBinaryData = nullptr;
	info.pVendorInfos = nullptr;

	auto device = Volt::RHI::GraphicsContext::GetDevice();
	VkDevice vkDevice = device->GetHandle<VkDevice>();

	VkResult result = Volt::RHI::vkGetDeviceFaultInfoEXT(vkDevice, &counts, &info);

	if (result == VK_INCOMPLETE)
	{
		Vector<VkDeviceFaultAddressInfoEXT> addressInfos(counts.addressInfoCount);
		Vector<VkDeviceFaultVendorInfoEXT> vendorInfos(counts.vendorInfoCount);
		Vector<uint8_t> vendorBinary(counts.vendorBinarySize);

		info.pAddressInfos = addressInfos.data();
		info.pVendorInfos = vendorInfos.data();
		info.pVendorBinaryData = vendorBinary.data();

		result = Volt::RHI::vkGetDeviceFaultInfoEXT(vkDevice, &counts, &info);
	}

	if (result == VK_SUCCESS)
	{
		VT_LOG(Error, "Device fault description {}\n", info.description);

		for (uint32_t i = 0; i < counts.addressInfoCount; ++i)
		{
			auto& a = info.pAddressInfos[i];

			VT_LOG(Error, "Fault address: {}, type: {}\n", (uint64_t)a.reportedAddress, GetAddressTypeStr(a.addressType));
		}
	}
#endif
}
