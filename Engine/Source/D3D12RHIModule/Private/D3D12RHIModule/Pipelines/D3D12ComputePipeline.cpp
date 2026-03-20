#include "dxpch.h"

#include "D3D12RHIModule/Pipelines/D3D12ComputePipeline.h"
#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"
#include "D3D12RHIModule/Utility/RootSignatureBuilder.h"

#include <RHIModule/RHIModule.h>

#include <CoreUtilities/Time/ScopedTimer.h>
#include <CoreUtilities/Math/Hash.h>

namespace Volt::RHI
{
	D3D12ComputePipeline::D3D12ComputePipeline(IntRef<Shader> shader)
		: m_shader(shader)
	{
		Invalidate();
	}

	D3D12ComputePipeline::~D3D12ComputePipeline()
	{
		Release();
	}

	void D3D12ComputePipeline::Invalidate()
	{
		Release();

		VT_ENSURE(m_shader);
		VT_ENSURE(m_shader->GetShaderStage() == ShaderStage::Compute);

		ScopedTimer scopedTimer{};

		// Create root signature
		{
			const ShaderParameterMap& shaderParameterMap = m_shader->GetParameterMap();

			RootSignatureBuilder rootSignatureBuilder;
			m_rootSignature = rootSignatureBuilder.BuildFromShaderResourceBindings(shaderParameterMap.GetResourceBindings(), shaderParameterMap.AccessesRayTracingTable());
			m_shaderParameterMap = shaderParameterMap;
		}

		// Create pipeline
		{
			D3D12Shader& d3d12Shader = m_shader->AsRef<D3D12Shader>();
			const ArrayView<uint32_t> shaderBinary = d3d12Shader.GetShaderBinary();

			D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc{};
			pipelineDesc.CachedPSO = { nullptr, 0 };
			pipelineDesc.NodeMask = 0;
			pipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
			pipelineDesc.CS = { shaderBinary.data(), shaderBinary.byte_size() };
			pipelineDesc.pRootSignature = m_rootSignature.rootSignature.Get();

			auto d3d12Device = GraphicsContext::GetDevice()->AsRef<D3D12GraphicsDevice>().GetDevice10();
			VT_D3D12_CHECK(d3d12Device->CreateComputePipelineState(&pipelineDesc, VT_D3D12_ID(m_pipeline)));
		}

		GenerateHash();
		VT_LOGC(Trace, LogD3D12RHI, "Created D3D12 Compute Pipeline in {} seconds!", scopedTimer.GetTime<Time::Seconds>());
	}

	bool D3D12ComputePipeline::IsValid() const
	{
		return m_pipeline != nullptr;
	}

	size_t D3D12ComputePipeline::GetHash() const
	{
		return m_hash;
	}

	void* D3D12ComputePipeline::GetHandleImpl() const
	{
		return m_pipeline.Get();
	}

	void D3D12ComputePipeline::Release()
	{
		if (!m_pipeline)
		{
			return;
		}

		RHIModule::GetInstance().DestroyResource([pipeline = m_pipeline, rootSignature = m_rootSignature.rootSignature]() mutable
		{
			pipeline.Reset();
			rootSignature.Reset();
		});

		m_pipeline.Reset();
		m_rootSignature.rootSignature.Reset();
	}

	void D3D12ComputePipeline::GenerateHash()
	{
		m_hash = m_shader->GetHash();

		m_hash = Math::HashCombine(m_hash, std::hash<void*>()(static_cast<void*>(m_pipeline.Get())));
		m_hash = Math::HashCombine(m_hash, std::hash<void*>()(static_cast<void*>(m_rootSignature.rootSignature.Get())));
	}

	const ShaderResourceBinding* D3D12ComputePipeline::GetResourceBindingFromName(const StringHash& name) const
	{
		const ShaderParameterMap::ResourceBindings& resourceBindings = m_shaderParameterMap.GetResourceBindings();
		for (const auto& [binding, nameHash] : resourceBindings)
		{
			if (nameHash == name)
			{
				return &binding;
			}
		}

		return nullptr;
	}

	IntRef<Shader> D3D12ComputePipeline::GetShader() const
	{
		return m_shader;
	}

	const ShaderParameterMap& D3D12ComputePipeline::GetShaderParameterMap() const
	{
		return m_shaderParameterMap;
	}

	bool D3D12ComputePipeline::HasInlineParameters() const
	{
		return false;
	}
}
