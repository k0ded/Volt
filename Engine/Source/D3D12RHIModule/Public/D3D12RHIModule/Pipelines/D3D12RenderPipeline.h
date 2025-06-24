#pragma once

#include "D3D12RHIModule/Common/ComPtr.h"

#include <RHIModule/Pipelines/RenderPipeline.h>

struct ID3D12PipelineState;

namespace Volt::RHI
{
	class D3D12RenderPipeline final : public RenderPipeline
	{
	public:
		D3D12RenderPipeline(const RenderPipelineCreateInfo& createInfo);
		~D3D12RenderPipeline() override;

		void Invalidate() override;
		RefPtr<Shader> GetShader() const override;
		bool IsValid() const override;
		size_t GetHash() const override;
		const ShaderResourceBinding* GetResourceBindingFromName(const StringHash& name, ShaderStage shaderStage) const override { return nullptr; }
		const Vector<ShaderParameterMap>& GetShaderParameterMaps() const override { static Vector<ShaderParameterMap> s; return s; }

		VT_NODISCARD VT_INLINE Topology GetTopology() const { return m_createInfo.topology; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void Release();
		void GenerateHash();

		RenderPipelineCreateInfo m_createInfo;
		ComPtr<ID3D12PipelineState> m_pipeline;
		size_t m_hash = 0;
	};
}
