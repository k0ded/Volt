#include "dxpch.h"

#include "D3D12RHIModule/Pipelines/D3D12RenderPipeline.h"
#include "D3D12RHIModule/Common/D3D12Helpers.h"
#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"

#include <RHIModule/Shader/ShaderUtility.h>
#include <RHIModule/RHIModule.h>

#include <CoreUtilities/Time/ScopedTimer.h>
#include <CoreUtilities/Math/Hash.h>

namespace Volt::RHI
{
	inline Vector<D3D12_INPUT_ELEMENT_DESC> CreateInputLayout(const BufferLayoutMap& vertexLayoutMap, const BufferLayout& instanceLayout, VertexBufferLayout& vertexBufferLayout)
	{
		Vector<D3D12_INPUT_ELEMENT_DESC> result{};

		uint32_t lastVertexBufferIndex = 0;

		for (const auto& [index, vertexLayout] : vertexLayoutMap)
		{
			for (const auto& element : vertexLayout.GetElements())
			{
				auto& d3d12Element = result.emplace_back();
				d3d12Element.SemanticName = element.name.c_str();
				d3d12Element.SemanticIndex = 0;
				d3d12Element.Format = Utility::VoltToD3D12ElementFormat(element.type);
				d3d12Element.InputSlot = index;
				d3d12Element.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
				d3d12Element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
				d3d12Element.InstanceDataStepRate = 0;
			}

			lastVertexBufferIndex = std::max(lastVertexBufferIndex, index);

			vertexBufferLayout.vertexBuffers.emplace_back(vertexLayout, index);
		}

		if (instanceLayout.IsValid())
		{
			lastVertexBufferIndex++;
			vertexBufferLayout.perInstanceVertexBuffer = { instanceLayout, lastVertexBufferIndex };

			for (const auto& element : instanceLayout.GetElements())
			{
				auto& d3d12Element = result.emplace_back();
				d3d12Element.SemanticName = element.name.c_str();
				d3d12Element.SemanticIndex = 0;
				d3d12Element.Format = Utility::VoltToD3D12ElementFormat(element.type);
				d3d12Element.InputSlot = lastVertexBufferIndex;
				d3d12Element.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
				d3d12Element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
				d3d12Element.InstanceDataStepRate = 1;
			}
		}

		return result;
	}

	inline Vector<D3D12_INPUT_ELEMENT_DESC> CreateInputLayoutFromShaders(const PipelineShadersVector& shaders, VertexBufferLayout& vertexBufferLayout)
	{
		// We will pick the first shader that contains a vertex layout (should only be one anyways)
		for (const auto shader : shaders)
		{
			const ShaderInfo& shaderInfo = shader->GetShaderInfo();

			if (!shaderInfo.vertexLayout.empty())
			{
				if (shaderInfo.vertexLayout.begin()->second.IsValid())
				{
					return CreateInputLayout(shaderInfo.vertexLayout, shaderInfo.instanceLayout, vertexBufferLayout);
				}
			}
		}

		// We allow no vertex layout
		return {};
	}

	D3D12RenderPipeline::D3D12RenderPipeline(const RenderPipelineCreateInfo& createInfo)
		: m_createInfo(createInfo)
	{
		Invalidate();
	}

	D3D12RenderPipeline::~D3D12RenderPipeline()
	{
		Release();
	}

	void D3D12RenderPipeline::Invalidate()
	{
		Release();

		ScopedTimer scopedTimer{};

		if (m_createInfo.enablePrimitiveRestart)
		{
			VT_ENSURE(m_createInfo.topology != Topology::TriangleList && m_createInfo.topology != Topology::LineList && m_createInfo.topology != Topology::PatchList && m_createInfo.topology != Topology::PointList);
		}

		VerifyShaderStages();

		// Create root signature
		bool anyAccessesRayTracingResourceTable = false;
		{
			Vector<ShaderParameterMap::ResourceBindings> shaderResourceBindings;

			for (const auto shader : m_createInfo.shaders)
			{
				const ShaderParameterMap& parameterMap = shader->GetParameterMap();

				m_shaderParameterMaps[GetDescriptorSetIndexFromShaderStage(shader->GetShaderStage())] = parameterMap;
				shaderResourceBindings.emplace_back(parameterMap.GetResourceBindings());

				anyAccessesRayTracingResourceTable |= parameterMap.AccessesRayTracingTable();
			}

			RootSignatureBuilder rootSignatureBuilder;
			m_rootSignature = rootSignatureBuilder.BuildFromShaderResourceBindings(shaderResourceBindings, anyAccessesRayTracingResourceTable);
		}

		// Create pipeline
		{
			Vector<D3D12_INPUT_ELEMENT_DESC> inputLayout = CreateInputLayoutFromShaders(m_createInfo.shaders, m_vertexBufferLayout);

			D3D12_RASTERIZER_DESC rasterizerDesc{};
			rasterizerDesc.FillMode = Utility::VoltToD3D12Fill(m_createInfo.fillMode);
			rasterizerDesc.CullMode = Utility::VoltToD3D12Cull(m_createInfo.cullMode);
			rasterizerDesc.FrontCounterClockwise = FALSE;
			rasterizerDesc.DepthBias = 0u;
			rasterizerDesc.DepthBiasClamp = m_createInfo.depthBiasClamp;
			rasterizerDesc.SlopeScaledDepthBias = m_createInfo.depthBiasSlopeFactor;
			rasterizerDesc.DepthClipEnable = TRUE;
			rasterizerDesc.MultisampleEnable = FALSE;
			rasterizerDesc.AntialiasedLineEnable = TRUE;
			rasterizerDesc.ForcedSampleCount = 0;
			rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

			D3D12_BLEND_DESC blendDesc{};
			blendDesc.AlphaToCoverageEnable = FALSE;
			blendDesc.IndependentBlendEnable = FALSE;

			for (size_t index = 0; index < m_createInfo.colorAttachmentFormats.size(); ++index)
			{
				blendDesc.RenderTarget[index].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
				blendDesc.RenderTarget[index].BlendEnable = m_createInfo.attachmentBlendStates[index].enabled;
				blendDesc.RenderTarget[index].SrcBlend = Utility::VoltToD3D12BlendFactor(m_createInfo.attachmentBlendStates[index].srcColorBlend);
				blendDesc.RenderTarget[index].SrcBlendAlpha = Utility::VoltToD3D12BlendFactor(m_createInfo.attachmentBlendStates[index].srcAlphaBlend);
				blendDesc.RenderTarget[index].DestBlend = Utility::VoltToD3D12BlendFactor(m_createInfo.attachmentBlendStates[index].dstColorBlend);
				blendDesc.RenderTarget[index].DestBlendAlpha = Utility::VoltToD3D12BlendFactor(m_createInfo.attachmentBlendStates[index].dstAlphaBlend);
				blendDesc.RenderTarget[index].BlendOp = Utility::VoltToD3D12BlendOp(m_createInfo.attachmentBlendStates[index].colorBlendOp);
				blendDesc.RenderTarget[index].BlendOpAlpha = Utility::VoltToD3D12BlendOp(m_createInfo.attachmentBlendStates[index].colorBlendOp);
			}

			D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
			depthStencilDesc.DepthFunc = Utility::VoltToD3D12CompareOp(m_createInfo.depthCompareOperator);
			depthStencilDesc.StencilEnable = FALSE;
			depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
			depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

			const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
			depthStencilDesc.FrontFace = defaultStencilOp;
			depthStencilDesc.BackFace = defaultStencilOp;

			switch (m_createInfo.depthMode)
			{
				case DepthMode::None:
				{
					depthStencilDesc.DepthEnable = FALSE;
					depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
					break;
				}

				case DepthMode::Read:
				{
					depthStencilDesc.DepthEnable = TRUE;
					depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
					break;
				}

				case DepthMode::Write:
				{
					depthStencilDesc.DepthEnable = FALSE;
					depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
					break;
				}

				case DepthMode::ReadWrite:
				{
					depthStencilDesc.DepthEnable = TRUE;
					depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
					break;
				}
			}

			D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc{};
			memset(&pipelineStateDesc, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
			pipelineStateDesc.InputLayout = { inputLayout.data(), static_cast<uint32_t>(inputLayout.size()) };
			pipelineStateDesc.pRootSignature = m_rootSignature.rootSignature.Get();
			pipelineStateDesc.RasterizerState = rasterizerDesc;

			for (const auto& shader : m_createInfo.shaders)
			{
				D3D12Shader& d3d12Shader = shader->AsRef<D3D12Shader>();

				const ArrayView<uint32_t> shaderBinary = d3d12Shader.GetShaderBinary();

				if (d3d12Shader.GetShaderStage() == ShaderStage::Vertex)
				{
					pipelineStateDesc.VS = { shaderBinary.data(), shaderBinary.byte_size() };
				}
				else if (d3d12Shader.GetShaderStage() == ShaderStage::Pixel)
				{
					pipelineStateDesc.PS = { shaderBinary.data(), shaderBinary.byte_size() };
				}
				else if (d3d12Shader.GetShaderStage() == ShaderStage::Hull)
				{
					pipelineStateDesc.HS = { shaderBinary.data(), shaderBinary.byte_size() };
				}
				else if (d3d12Shader.GetShaderStage() == ShaderStage::Domain)
				{
					pipelineStateDesc.DS = { shaderBinary.data(), shaderBinary.byte_size() };
				}
				else if (d3d12Shader.GetShaderStage() == ShaderStage::Geometry)
				{
					pipelineStateDesc.GS = { shaderBinary.data(), shaderBinary.byte_size() };
				}
			}

			pipelineStateDesc.BlendState = blendDesc;
			pipelineStateDesc.DepthStencilState = depthStencilDesc;
			pipelineStateDesc.SampleMask = UINT_MAX;
			pipelineStateDesc.PrimitiveTopologyType = Utility::VoltToD3D12Topology(m_createInfo.topology);
			pipelineStateDesc.SampleDesc.Count = 1;
			pipelineStateDesc.SampleDesc.Quality = 0;

			// Depth format
			{
				DXGI_FORMAT dxgiFormat = ConvertFormatToD3D12Format(m_createInfo.depthAttachmentFormat);
				if (Utility::IsFormatTypeless(dxgiFormat))
				{
					pipelineStateDesc.DSVFormat = Utility::GetDSVFormatFromTypeless(dxgiFormat);
				}
				else
				{
					pipelineStateDesc.DSVFormat = dxgiFormat;
				}
			}

			for (size_t i = 0; i < m_createInfo.colorAttachmentFormats.size(); ++i)
			{
				pipelineStateDesc.RTVFormats[i] = ConvertFormatToD3D12Format(m_createInfo.colorAttachmentFormats[i]);
			}

			pipelineStateDesc.NumRenderTargets = static_cast<uint32_t>(m_createInfo.colorAttachmentFormats.size()) + (m_createInfo.depthAttachmentFormat != PixelFormat::UNDEFINED);

			auto d3d12Device = GraphicsContext::GetDevice()->AsRef<D3D12GraphicsDevice>().GetDevice10();

			VT_D3D12_CHECK(d3d12Device->CreateGraphicsPipelineState(&pipelineStateDesc, VT_D3D12_ID(m_pipeline)));
			GenerateHash();
			VT_LOGC(Trace, LogD3D12RHI, "Created D3D12 Render Pipeline in {} seconds!", scopedTimer.GetTime<Time::Seconds>());
		}
	}

	bool D3D12RenderPipeline::IsValid() const
	{
		return m_pipeline != nullptr;
	}

	size_t D3D12RenderPipeline::GetHash() const
	{
		return m_hash;
	}

	const ShaderResourceBinding* D3D12RenderPipeline::GetResourceBindingFromName(const StringHash& name, ShaderStage shaderStage) const
	{
		auto& shaderParameterMap = m_shaderParameterMaps[GetDescriptorSetIndexFromShaderStage(shaderStage)];

		const ShaderParameterMap::ResourceBindings& resourceBindingsMap = shaderParameterMap.GetResourceBindings();
		for (const auto& [binding, nameHash] : resourceBindingsMap)
		{
			if (nameHash == name)
			{
				return &binding;

				// We can break here because there is only max one of each shader stage per pipeline
				break;
			}
		}

		return nullptr;
	}

	ArrayView<ShaderParameterMap> D3D12RenderPipeline::GetShaderParameterMaps() const
	{
		return m_shaderParameterMaps;
	}

	const PipelineShadersVector& D3D12RenderPipeline::GetShaders() const
	{
		return m_createInfo.shaders;
	}

	const VertexBufferLayout& D3D12RenderPipeline::GetVertexBufferLayout() const
	{
		return m_vertexBufferLayout;
	}

	void* D3D12RenderPipeline::GetHandleImpl() const
	{
		return m_pipeline.Get();
	}

	void D3D12RenderPipeline::Release()
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

		m_pipeline = nullptr;
		m_rootSignature.rootSignature = nullptr;
	}

	void D3D12RenderPipeline::GenerateHash()
	{
		m_hash = 0;
		for (const auto& shader : m_createInfo.shaders)
		{
			m_hash = Math::HashCombine(m_hash, shader->GetHash());
		}

		m_hash = Math::HashCombine(m_hash, std::hash<void*>()(static_cast<void*>(m_pipeline.Get())));
		m_hash = Math::HashCombine(m_hash, std::hash<void*>()(static_cast<void*>(m_rootSignature.rootSignature.Get())));
	}

	void D3D12RenderPipeline::VerifyShaderStages()
	{
		bool foundVertexShader = false;
		bool foundMeshShader = false;
		bool foundAmplificationShader = false;
		bool foundPixelShader = false;

		for (const auto& shader : m_createInfo.shaders)
		{
			switch (shader->GetShaderStage())
			{
				case ShaderStage::Vertex: VT_ENSURE(!foundVertexShader); foundVertexShader = true; break;
				case ShaderStage::Mesh: VT_ENSURE(!foundMeshShader); foundMeshShader = true; break;
				case ShaderStage::Amplification: VT_ENSURE(!foundAmplificationShader); foundAmplificationShader = true; break;
				case ShaderStage::Pixel: VT_ENSURE(!foundPixelShader); foundPixelShader = true; break;
			}
		}

		if (foundPixelShader)
		{
			VT_ENSURE(foundVertexShader || foundMeshShader);
		}

		if (foundAmplificationShader)
		{
			VT_ENSURE(foundMeshShader && !foundVertexShader);
		}

		if (foundVertexShader)
		{
			VT_ENSURE(!foundAmplificationShader && !foundMeshShader);
		}

		if (foundMeshShader)
		{
			VT_ENSURE(!foundVertexShader);
		}

		VT_ENSURE_MSG(!foundMeshShader && !foundAmplificationShader, "Mesh shaders are not implemented!");
	}

	bool D3D12RenderPipeline::HasInlineParameters() const
	{
		return false;
	}
}
