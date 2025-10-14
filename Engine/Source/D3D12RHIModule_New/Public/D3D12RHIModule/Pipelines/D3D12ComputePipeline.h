#pragma once

#include "D3D12RHIModule/Common/ComPtr.h"
#include "D3D12RHIModule/Utility/RootSignatureBuilder.h"

#include <RHIModule/Pipelines/ComputePipeline.h>
#include <RHIModule/Shader/Shader.h>

namespace Volt::RHI
{
	class D3D12ComputePipeline final : public ComputePipeline
	{
	public:
		D3D12ComputePipeline(RefPtr<Shader> shader);
		~D3D12ComputePipeline() override;

		void Invalidate() override;
		RefPtr<Shader> GetShader() const override;
		bool IsValid() const override;
		size_t GetHash() const override;
		const ShaderResourceBinding* GetResourceBindingFromName(const StringHash& name) const override;
		const ShaderParameterMap& GetShaderParameterMap() const override;

		VT_NODISCARD VT_INLINE const RootSignatureBuilder::RootSignature& GetRootSignature() const { return m_rootSignature; }

	protected:
		void* GetHandleImpl() const override;
	
	private:
		void Release();
		void GenerateHash();

		RefPtr<Shader> m_shader;
		size_t m_hash;

		ComPtr<ID3D12PipelineState> m_pipeline;

		ShaderParameterMap m_shaderParameterMap;
		RootSignatureBuilder::RootSignature m_rootSignature;
	};
}
