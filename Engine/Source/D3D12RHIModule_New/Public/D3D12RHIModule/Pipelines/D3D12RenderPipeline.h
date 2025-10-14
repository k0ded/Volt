#pragma once

#include "D3D12RHIModule/Common/ComPtr.h"
#include "D3D12RHIModule/Utility/RootSignatureBuilder.h"

#include <RHIModule/Pipelines/RenderPipeline.h>

struct ID3D12PipelineState;

namespace Volt::RHI
{
	class D3D12RenderPipeline : public RenderPipeline
	{
	public:
		D3D12RenderPipeline(const RenderPipelineCreateInfo& createInfo);
		~D3D12RenderPipeline() override;

		void Invalidate() override;
		bool IsValid() const override;
		size_t GetHash() const override;
		const ShaderResourceBinding* GetResourceBindingFromName(const StringHash& name, ShaderStage shaderStage) const override;
		ArrayView<ShaderParameterMap> GetShaderParameterMaps() const override;
		const Vector<RefPtr<Shader>>& GetShaders() const override;
		const VertexBufferLayout& GetVertexBufferLayout() const override;

		VT_NODISCARD VT_INLINE const RootSignatureBuilder::RootSignature& GetRootSignature() const { return m_rootSignature; }
		VT_NODISCARD VT_INLINE Topology GetTopology() const { return m_createInfo.topology; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void Release();
		void GenerateHash();
		void VerifyShaderStages();

		RenderPipelineCreateInfo m_createInfo{};
		size_t m_hash;

		Array<ShaderParameterMap, GetNumShaderStages()> m_shaderParameterMaps;
		VertexBufferLayout m_vertexBufferLayout;
		RootSignatureBuilder::RootSignature m_rootSignature;

		ComPtr<ID3D12PipelineState> m_pipeline;
	};
}
