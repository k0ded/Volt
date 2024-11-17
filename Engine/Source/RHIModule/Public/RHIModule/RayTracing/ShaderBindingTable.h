#pragma once

#include "RHIModule/Core/RHIInterface.h"

namespace Volt::RHI
{
	class RayTracingPipeline;
	class VTRHI_API ShaderBindingTable : public RHIInterface
	{
	public:
		static RefPtr<ShaderBindingTable> Create(RefPtr<RayTracingPipeline> pipeline);

	protected:
		ShaderBindingTable() = default;
		virtual ~ShaderBindingTable() = default;
	};
}
