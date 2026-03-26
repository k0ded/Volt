#pragma once

#include <RHIModule/RayTracing/AccelerationStructure.h>

#include <CoreUtilities/Pointers/Ref.h>

namespace Volt
{
	namespace RHI
	{
		class Buffer;
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
		IntRef<RHI::Buffer> vertexPositionsBuffer;
		IntRef<RHI::Buffer> indexBuffer;

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
		IntRef<RHI::AccelerationStructure> m_accelerationStructure;
	};
}
