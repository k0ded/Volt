#pragma once

#include <RHIModule/RayTracing/RayTracingCommon.h>

#include <CoreUtilities/EnumUtils.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI::Utility
{
	inline constexpr VkGeometryTypeKHR GetGeometryType(AccelerationStructureGeometryType type)
	{
		switch (type)
		{
			case AccelerationStructureGeometryType::Triangles: return VK_GEOMETRY_TYPE_TRIANGLES_KHR;
			case AccelerationStructureGeometryType::AABBs: return VK_GEOMETRY_TYPE_AABBS_KHR;
			case AccelerationStructureGeometryType::Instances: return VK_GEOMETRY_TYPE_INSTANCES_KHR;
		}

		VT_ENSURE(false);
		VT_UNREACHABLE;
	}

	inline constexpr VkGeometryFlagsKHR GetGeometryFlags(AccelerationStructureGeometryFlags flags)
	{
		VkGeometryFlagsKHR result = static_cast<VkGeometryFlagsKHR>(0);

		if (EnumValueContainsFlag(flags, AccelerationStructureGeometryFlags::Opaque))
		{
			result |= VK_GEOMETRY_OPAQUE_BIT_KHR;
		}

		if (EnumValueContainsFlag(flags, AccelerationStructureGeometryFlags::NoDuplicateAnyHitInvocation))
		{
			result |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
		}

		return result;
	}

	inline constexpr VkAccelerationStructureTypeKHR GetAccelerationStructureType(AccelerationStructureType type)
	{
		switch (type)
		{
			case AccelerationStructureType::TopLevel: return VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
			case AccelerationStructureType::BottomLevel: return VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			case AccelerationStructureType::Generic: return VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR;
		}

		VT_ENSURE(false);
		VT_UNREACHABLE;
	}

	inline constexpr VkBuildAccelerationStructureFlagsKHR GetAccelerationStructureBuildFlags(AccelerationStructureBuildFlags flags)
	{
		VkBuildAccelerationStructureFlagsKHR result = static_cast<VkBuildAccelerationStructureFlagsKHR>(0);

		if (EnumValueContainsFlag(flags, AccelerationStructureBuildFlags::AllowUpdate))
		{
			result |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		}

		if (EnumValueContainsFlag(flags, AccelerationStructureBuildFlags::AllowCompaction))
		{
			result |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
		}

		if (EnumValueContainsFlag(flags, AccelerationStructureBuildFlags::PreferFastTrace))
		{
			result |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		}

		if (EnumValueContainsFlag(flags, AccelerationStructureBuildFlags::PreferFastBuild))
		{
			result |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
		}

		if (EnumValueContainsFlag(flags, AccelerationStructureBuildFlags::LowMemory))
		{
			result |= VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR;
		}

		return result;
	}

	inline constexpr VkBuildAccelerationStructureModeKHR GetAccelerationStructureBuildMode(AccelerationStructureBuildMode buildMode)
	{
		switch (buildMode)
		{
			case AccelerationStructureBuildMode::Build: return VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
			case AccelerationStructureBuildMode::Update: return VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
		}
		VT_ENSURE(false);
		VT_UNREACHABLE;
	}
}
