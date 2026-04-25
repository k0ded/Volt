#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/RayTracing/RayTracingCommon.h"
#include "RHIModule/Descriptors/BindlessIndex.h"

namespace Volt::RHI
{
	struct AccelerationStructureCreateInfo
	{
		Vector<AccelerationStructureGeometryInfo> geometries;
		AccelerationStructureBuildFlags flags;
		AccelerationStructureType type;
	};

	class VTRHI_API AccelerationStructure : public RHIInterface
	{
	public:
		virtual uint64_t GetDeviceAddress() const = 0;
		virtual BindlessIndex GetBindlessIndex() const = 0;
		
		static IntRef<AccelerationStructure> Create(const AccelerationStructureCreateInfo& createInfo);

	protected:
		AccelerationStructure() = default;
		virtual ~AccelerationStructure() = default;
	};
}
