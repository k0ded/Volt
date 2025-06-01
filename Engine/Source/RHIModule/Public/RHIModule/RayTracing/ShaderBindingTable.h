#pragma once

#include "RHIModule/Core/RHIInterface.h"

namespace Volt::RHI
{
	class RayTracingPipeline;
	class Shader2;

	class VTRHI_API ShaderBindingTable : public RHIInterface
	{
	public:
		virtual void Invalidate() = 0;
		virtual bool IsShaderInTable(RefPtr<Shader2> shader) const = 0;

		static RefPtr<ShaderBindingTable> Create(RefPtr<RayTracingPipeline> pipeline);

	protected:
		ShaderBindingTable() = default;
		virtual ~ShaderBindingTable() = default;
	};
}
