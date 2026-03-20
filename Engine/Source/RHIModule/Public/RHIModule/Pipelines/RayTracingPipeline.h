#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Shader/Shader.h"

#include <CoreUtilities/Containers/Vector.h>

namespace Volt::RHI
{ 
	struct RayTracingPipelineCreateInfo
	{
		Vector<IntRef<Shader>> rayGenTable;
		Vector<IntRef<Shader>> missTable;
		Vector<IntRef<Shader>> closestHitTable;
		Vector<IntRef<Shader>> anyHitTable;
		Vector<IntRef<Shader>> intersectionTable;
		Vector<IntRef<Shader>> callableTable;
	};

	class VTRHI_API RayTracingPipeline : public RHIInterface
	{
	public:
		virtual void Invalidate() = 0;
		virtual bool IsValid() const = 0;
		virtual bool IsShaderInPipeline(IntRef<Shader> shader) const = 0;

		static IntRef<RayTracingPipeline> Create(const RayTracingPipelineCreateInfo& createInfo);

	protected:
		RayTracingPipeline() = default;
		virtual ~RayTracingPipeline() = default;
	};
}
