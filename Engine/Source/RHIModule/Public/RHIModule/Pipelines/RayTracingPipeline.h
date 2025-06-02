#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Shader/Shader.h"

#include <CoreUtilities/Containers/Vector.h>

namespace Volt::RHI
{ 
	struct RayTracingPipelineCreateInfo
	{
		Vector<RefPtr<Shader>> rayGenTable;
		Vector<RefPtr<Shader>> missTable;
		Vector<RefPtr<Shader>> closestHitTable;
		Vector<RefPtr<Shader>> anyHitTable;
		Vector<RefPtr<Shader>> intersectionTable;
		Vector<RefPtr<Shader>> callableTable;
	};

	struct ShaderUniforms;

	class VTRHI_API RayTracingPipeline : public RHIInterface
	{
	public:
		virtual void Invalidate() = 0;
		virtual bool IsValid() const = 0;
		virtual bool IsShaderInPipeline(RefPtr<Shader> shader) const = 0;
		virtual const ShaderUniforms& GetRenderGraphConstants() const = 0;

		static RefPtr<RayTracingPipeline> Create(const RayTracingPipelineCreateInfo& createInfo);

	protected:
		RayTracingPipeline() = default;
		virtual ~RayTracingPipeline() = default;
	};
}
