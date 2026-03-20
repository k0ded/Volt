#pragma once

#include "RHIModule/Core/RHIInterface.h"

namespace Volt::RHI
{
	class RayTracingPipeline;
	class Shader;

	class VTRHI_API ShaderBindingTable : public RHIInterface
	{
	public:
		virtual void Invalidate() = 0;
		virtual bool IsShaderInTable(IntRef<Shader> shader) const = 0;

		static IntRef<ShaderBindingTable> Create(IntRef<RayTracingPipeline> pipeline);

	protected:
		ShaderBindingTable() = default;
		virtual ~ShaderBindingTable() = default;
	};
}
