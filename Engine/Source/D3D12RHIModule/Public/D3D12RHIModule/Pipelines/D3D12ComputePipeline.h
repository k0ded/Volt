#pragma once

#include "D3D12RHIModule/Common/ComPtr.h"

#include <RHIModule/Pipelines/ComputePipeline.h>
#include <RHIModule/Shader/Shader.h>

struct ID3D12PipelineState;

namespace Volt::RHI
{
	class D3D12ComputePipeline : public ComputePipeline
	{
	public:
		D3D12ComputePipeline(RefPtr<Shader> shader, bool useGlobalResources);
		~D3D12ComputePipeline() override;

		void Invalidate() override;
		RefPtr<Shader> GetShader() const override;
		RefPtr<Shader> GetShader2() const override { return nullptr; }
		bool IsValid() const override;
		size_t GetHash() const override;
		const ShaderResourceBinding* GetResourceBindingFromName(const StringHash& name) const override { return nullptr; }
		const ShaderParameterMap& GetShaderParameterMap() const override { static ShaderParameterMap s;  return s; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void Release();
		void GenerateHash();

		ComPtr<ID3D12PipelineState> m_pipeline;

		RefPtr<Shader> m_shader;
		size_t m_hash = 0;
		bool m_useGlobalResouces = false;
	};
}
