#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Shader/Shader2.h"

#include <CoreUtilities/Containers/Vector.h>

namespace Volt::RHI
{ 
	struct RayTracingPipelineCreateInfo
	{
		Vector<RefPtr<Shader2>> rayGenTable;
		Vector<RefPtr<Shader2>> missTable;
		Vector<RefPtr<Shader2>> closestHitTable;
		Vector<RefPtr<Shader2>> anyHitTable;
		Vector<RefPtr<Shader2>> intersectionTable;
		Vector<RefPtr<Shader2>> callableTable;
	};

	struct ShaderUniforms;

	class VTRHI_API RayTracingPipeline : public RHIInterface
	{
	public:
		virtual void Invalidate() = 0;
		virtual bool IsValid() const = 0;
		virtual bool IsShaderInPipeline(RefPtr<Shader2> shader) const = 0;
		virtual const ShaderUniforms& GetRenderGraphConstants() const = 0;

		static RefPtr<RayTracingPipeline> Create(const RayTracingPipelineCreateInfo& createInfo);

	protected:
		RayTracingPipeline() = default;
		virtual ~RayTracingPipeline() = default;
	};
}
