#pragma once

#include <RHIModule/RayTracing/AccelerationStructure.h>

namespace Volt
{
	namespace RHI
	{
		class StorageBuffer;
	}

	struct RayTracingSceneGeometryInfo
	{
		uint32_t indexCount;
		uint32_t vertexCount;
		uint32_t indexOffset;
		uint32_t vertexOffset;
	};

	struct RayTracingSceneGeometryCreateInfo
	{
		RefPtr<RHI::StorageBuffer> vertexPositionsBuffer;
		RefPtr<RHI::StorageBuffer> indexBuffer;

		Vector<RayTracingSceneGeometryInfo> geometries;
	};

	class RayTracingSceneGeometry
	{
	public:
		RayTracingSceneGeometry(const RayTracingSceneGeometryCreateInfo& createInfo);
		~RayTracingSceneGeometry() = default;

		uint64_t GetAccelerationStructureDeviceAddress() const;

		static Ref<RayTracingSceneGeometry> Create(const RayTracingSceneGeometryCreateInfo& createInfo);

	private:
		RefPtr<RHI::AccelerationStructure> m_accelerationStructure;
	};
}
