#pragma once

#include "RHIModule/Core/RHICommon.h"

#include <CoreUtilities/Core.h>

#include <glm/glm.hpp>

#include <span>

namespace Volt::RHI
{
	enum class AccelerationStructureGeometryType : uint8_t
	{
		Triangles,
		AABBs,
		Instances
	};

	enum class AccelerationStructureGeometryFlags : uint8_t
	{
		None = 0,
		Opaque = BIT(0),
		NoDuplicateAnyHitInvocation = BIT(1)
	};

	VT_SETUP_ENUM_CLASS_OPERATORS(AccelerationStructureGeometryFlags);

	enum class AccelerationStructureType : uint8_t
	{
		TopLevel,
		BottomLevel,
		Generic
	};

	enum class AccelerationStructureBuildFlags : uint8_t
	{
		None = 0,
		AllowUpdate = BIT(0),
		AllowCompaction = BIT(1),
		PreferFastTrace = BIT(2),
		PreferFastBuild = BIT(3),
		LowMemory = BIT(4)
	};

	VT_SETUP_ENUM_CLASS_OPERATORS(AccelerationStructureBuildFlags);

	enum class AccelerationStructureBuildMode : uint8_t
	{
		Build,
		Update
	};

	enum class AccelerationStructureGeometryInstanceFlags : uint8_t
	{
		None = 0,
		TriangleFaceCullDistable = BIT(0),
		TriangleFlipFacing = BIT(1),
		ForceOpaque = BIT(2),
		ForceNoOpaque = BIT(3),
	};

	class Buffer;
	class AccelerationStructure;

	// Copied from vulkan_core.h. Might not work with D3D12.
	struct AccelerationStructureInstance
	{
		glm::mat3x4 transform;

		uint32_t instanceCustomIndex : 24;
		uint32_t mask : 8;
		uint32_t instanceShaderBindingTableRecordOffset : 24;
		uint32_t flags : 8; // Should be AccelerationStructureGeometryInstanceFlags, but doesn't work with bitfields.
		uint64_t accelerationStructureReference;
	};

	struct AccelerationStructureGeometryInfo
	{
		RefPtr<Buffer> vertexPositionsBuffer;
		RefPtr<Buffer> indexBuffer;
		RefPtr<Buffer> instancesBuffer;

		PixelFormat vertexFormat;
		uint32_t vertexStride;
		uint32_t vertexCount;
		uint32_t indexCount;

		IndexType indexType;
		AccelerationStructureGeometryType geometryType;
		AccelerationStructureGeometryFlags flags;
	};

	struct AccelerationStructureBuildGeometryInfo
	{
		Vector<AccelerationStructureGeometryInfo> geometries;

		RefPtr<AccelerationStructure> srcAccelerationStructure;
		RefPtr<AccelerationStructure> dstAccelerationStructure;

		AccelerationStructureType type;
		AccelerationStructureBuildFlags flags;
		AccelerationStructureBuildMode mode;
	};

	struct AccelerationStructureBuildRange
	{
		uint32_t primitiveCount;
		uint32_t primitiveOffset;
		uint32_t firstVertex;
		uint32_t transformOffset;
	};

	class AccelerationStructureBuildRanges
	{
	public:
		VT_INLINE AccelerationStructureBuildRange& AddRange()
		{
			return m_ranges.emplace_back();
		}

		VT_NODISCARD VT_INLINE std::span<const AccelerationStructureBuildRange> GetRanges() const { return m_ranges; }

	private:
		Vector<AccelerationStructureBuildRange> m_ranges;
	};
}
