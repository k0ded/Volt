#pragma once

#include "RHIModule/Core/RHIInterface.h"

namespace Volt::RHI
{
	class Shader;
	class Shader2;

	class VTRHI_API ComputePipeline : public RHIInterface
	{
	public:
		virtual void Invalidate() = 0;
		virtual RefPtr<Shader> GetShader() const = 0;
		virtual bool IsValid() const = 0;
		virtual size_t GetHash() const = 0;

		static RefPtr<ComputePipeline> Create(RefPtr<Shader> shader, bool useGlobalResources = true);
		static RefPtr<ComputePipeline> Create(RefPtr<Shader2> shader);

	protected:
		ComputePipeline() = default;
		virtual ~ComputePipeline() = default;
	};
}

