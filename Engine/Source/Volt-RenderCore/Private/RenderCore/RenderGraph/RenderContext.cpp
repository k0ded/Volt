#include "rcpch.h"
#include "RenderCore/RenderGraph/RenderContext.h"

#include "RenderCore/RenderGraph/RenderGraph.h"
#include "RenderCore/Shader/BatchedShaderParameters.h"
#include "RenderCore/Shader/PipelineStateCache.h"

#include <RHIModule/Buffers/UniformBuffer.h>
#include <RHIModule/Buffers/Buffer.h>
#include <RHIModule/Images/ImageView.h>
#include <RHIModule/RHIFeatures.h>
#include <RHIModule/Globals.h>

namespace Volt
{
#ifdef VT_ENABLE_RENDERGRAPH_VALIDATION
	void ValidateClearUAV(RGPassRef pass)
	{
		const RenderGraphPassFlags passFlags = pass->GetFlags();
		VT_ENSURE(EnumValueContainsFlag(passFlags, RenderGraphPassFlags::Clear));
	}
#endif

	RenderContext::RenderContext(RenderGraph& renderGraph, RGPassRef currentPass, IntRef<RHI::CommandBuffer> commandBuffer, RenderGraphShaderParameterUniformBuffer& shaderParameterUniformBuffer)
		: m_commandBuffer(commandBuffer),
		m_renderGraph(renderGraph),
		m_currentPass(currentPass),
		m_shaderParameterUniformBuffer(shaderParameterUniformBuffer)
	{
		VT_UNUSED(m_renderGraph);
	}

	void RenderContext::Flush(IntRef<RHI::Fence> fence)
	{
		//m_commandBuffer->Flush(fence);
	}

	void RenderContext::BeginRendering(const RenderingInfo& renderingInfo)
	{
		VT_PROFILE_FUNCTION();

		m_commandBuffer->SetViewports({ renderingInfo.viewport });
		m_commandBuffer->SetScissors({ renderingInfo.scissor });
		m_commandBuffer->BeginRendering(renderingInfo.renderingInfo);

		m_activeRenderingInfo = renderingInfo;
		m_isWithinRenderingScope = true;
	}

	void RenderContext::EndRendering()
	{
		VT_PROFILE_FUNCTION();

		m_isWithinRenderingScope = false;
		m_activeRenderingInfo = {};
		m_commandBuffer->EndRendering();
	}

	const RenderingInfo RenderContext::CreateRenderingInfo(const uint32_t width, const uint32_t height, const ShaderParameterRenderTargetBindings& rtBindings)
	{
		VT_PROFILE_FUNCTION();

		RHI::Rect2D scissor = { 0, 0, width, height };
		RHI::Viewport viewport{};
		viewport.width = static_cast<float>(width);
		viewport.height = static_cast<float>(height);
		viewport.x = 0.f;
		viewport.y = 0.f;
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		InlineVector<RHI::AttachmentInfo, RHI::MAX_COLOR_ATTACHMENT_COUNT> colorAttachments;
		RHI::AttachmentInfo depthAttachment{};

		for (size_t i = 0; i < RHI::MAX_COLOR_ATTACHMENT_COUNT; ++i)
		{
			const ShaderParameterRenderTargetDecl& rtDecl = rtBindings.renderTargets[i];

			if (rtDecl.texture != nullptr)
			{
				RHI::ImageViewDesc viewDesc{};
				viewDesc.baseMipLevel = rtDecl.subResourceRange.baseMipLevel;
				viewDesc.baseArrayLayer = rtDecl.subResourceRange.baseArrayLayer;
				viewDesc.mipCount = rtDecl.subResourceRange.mipCount;
				viewDesc.layerCount = rtDecl.subResourceRange.layerCount;

				IntRef<RHI::ImageView> view = rtDecl.texture->GetRHIResource()->GetOrCreateView(viewDesc);

				RHI::AttachmentInfo& attachment = colorAttachments.emplace_back();
				attachment.clearMode = RHI::ClearMode::Clear;
				attachment.clearColor = { 0.f, 0.f, 0.f, 0.f };
				attachment.view = view;
			}
		}

		const ShaderParameterRenderTargetDecl& depthDecl = rtBindings.depthTarget;

		if (depthDecl.texture != nullptr)
		{
			RHI::ImageViewDesc viewDesc{};
			viewDesc.baseMipLevel = depthDecl.subResourceRange.baseMipLevel;
			viewDesc.baseArrayLayer = depthDecl.subResourceRange.baseArrayLayer;
			viewDesc.mipCount = depthDecl.subResourceRange.mipCount;
			viewDesc.layerCount = depthDecl.subResourceRange.layerCount;

			depthAttachment.view = depthDecl.texture->GetRHIResource()->GetOrCreateView(viewDesc);
			depthAttachment.clearMode = RHI::ClearMode::Clear;
			depthAttachment.clearColor = { 0.f };
		}

		RHI::RenderingInfo renderingInfo{};
		renderingInfo.colorAttachments = colorAttachments;
		renderingInfo.depthAttachmentInfo = depthAttachment;
		renderingInfo.renderArea = scissor;

		RenderingInfo result{};
		result.renderingInfo = renderingInfo;
		result.scissor = scissor;
		result.viewport = viewport;

		return result;
	}

	void RenderContext::FillRenderingAttachmentDeclaration(RHI::RenderingAttachmentDeclaration& outDeclaration) const
	{
		outDeclaration.colorAttachmentFormats.resize(m_activeRenderingInfo.renderingInfo.colorAttachments.size());

		for (size_t i = 0; i < m_activeRenderingInfo.renderingInfo.colorAttachments.size(); ++i)
		{
			outDeclaration.colorAttachmentFormats[i] = m_activeRenderingInfo.renderingInfo.colorAttachments[i].view->GetFormat();
		}

		outDeclaration.depthAttachmentFormat = m_activeRenderingInfo.renderingInfo.depthAttachmentInfo.view ? m_activeRenderingInfo.renderingInfo.depthAttachmentInfo.view->GetFormat() : RHI::PixelFormat::UNDEFINED;
	}

	void RenderContext::DispatchMeshTasks(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ)
	{
		BindShaderBindings();

		m_commandBuffer->DispatchMeshTasks(groupCountX, groupCountY, groupCountZ);
	}

	void RenderContext::DispatchMeshTasksIndirect(RGBufferRef commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride)
	{
		BindShaderBindings();

		IntRef<RHI::Buffer> rhiCommandsBuffer = commandsBuffer->GetRHIResource()->GetRHIBuffer();
		m_commandBuffer->DispatchMeshTasksIndirect(rhiCommandsBuffer, offset, drawCount, stride);
	}

	void RenderContext::DispatchMeshTasksIndirectCount(RGBufferRef commandsBuffer, const size_t offset, RGBufferRef countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride)
	{
		BindShaderBindings();

		IntRef<RHI::Buffer> rhiCommandsBuffer = commandsBuffer->GetRHIResource()->GetRHIBuffer();
		IntRef<RHI::Buffer> rhiCountBuffer = countBuffer->GetRHIResource()->GetRHIBuffer();
		m_commandBuffer->DispatchMeshTasksIndirectCount(rhiCommandsBuffer, offset, rhiCountBuffer, countBufferOffset, maxDrawCount, stride);
	}

	void RenderContext::Dispatch(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ)
	{
		BindShaderBindings();

		m_commandBuffer->Dispatch(groupCountX, groupCountY, groupCountZ);
	}

	void RenderContext::DispatchIndirect(RGBufferRef commandsBuffer, const size_t offset)
	{
		BindShaderBindings();

		IntRef<RHI::Buffer> rhiCommandsBuffer = commandsBuffer->GetRHIResource()->GetRHIBuffer();
		m_commandBuffer->DispatchIndirect(rhiCommandsBuffer, offset);
	}

	void RenderContext::DrawIndirectCount(RGBufferRef commandsBuffer, const size_t offset, RGBufferRef countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride)
	{
		BindShaderBindings();

		IntRef<RHI::Buffer> rhiCommandsBuffer = commandsBuffer->GetRHIResource()->GetRHIBuffer();
		IntRef<RHI::Buffer> rhiCountBuffer = countBuffer->GetRHIResource()->GetRHIBuffer();
		m_commandBuffer->DispatchMeshTasksIndirectCount(rhiCommandsBuffer, offset, rhiCountBuffer, countBufferOffset, maxDrawCount, stride);
	}

	void RenderContext::DrawIndexedIndirect(RGBufferRef commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride)
	{
		BindShaderBindings();

		IntRef<RHI::Buffer> rhiCommandsBuffer = commandsBuffer->GetRHIResource()->GetRHIBuffer();
		m_commandBuffer->DrawIndexedIndirect(rhiCommandsBuffer, offset, drawCount, stride);
	}

	void RenderContext::DrawIndexed(const uint32_t indexCount, const uint32_t instanceCount, const uint32_t firstIndex, const uint32_t vertexOffset, const uint32_t firstInstance)
	{
		BindShaderBindings();

		m_commandBuffer->DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void RenderContext::Draw(const uint32_t vertexCount, const uint32_t instanceCount, const uint32_t firstVertex, const uint32_t firstInstance)
	{
		BindShaderBindings();
	
		m_commandBuffer->Draw(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void RenderContext::ClearUAV(RGTextureUAVRef textureUAV, const glm::uvec4& clearValues)
	{
#ifdef VT_ENABLE_RENDERGRAPH_VALIDATION
		ValidateClearUAV(m_currentPass);
#endif
		m_commandBuffer->ClearImageView(textureUAV->GetRHIView(), std::array<uint32_t, 4>{ clearValues[0], clearValues[1], clearValues[2], clearValues[3] });
	}

	void RenderContext::ClearUAV(RGTextureUAVRef textureUAV, const glm::vec4& clearValues)
	{
#ifdef VT_ENABLE_RENDERGRAPH_VALIDATION
		ValidateClearUAV(m_currentPass);
#endif
		m_commandBuffer->ClearImageView(textureUAV->GetRHIView(), std::array<float, 4>{ clearValues[0], clearValues[1], clearValues[2], clearValues[3] });
	}

	void RenderContext::ClearUAV(RGBufferUAVRef bufferUAV, const uint32_t clearValue)
	{
#ifdef VT_ENABLE_RENDERGRAPH_VALIDATION
		ValidateClearUAV(m_currentPass);
#endif
		m_commandBuffer->ClearBufferView(bufferUAV->GetRHIView(), clearValue);
	}

	void RenderContext::ClearUAV(RGBufferUAVRef bufferUAV, const float clearValue)
	{
#ifdef VT_ENABLE_RENDERGRAPH_VALIDATION
		ValidateClearUAV(m_currentPass);
#endif
		m_commandBuffer->ClearBufferView(bufferUAV->GetRHIView(), clearValue);
	}

	void RenderContext::SetPipelineState(const GraphicsPipelineState& pipelineState)
	{
		m_currentRenderPipeline = CreateRenderPipeline(pipelineState);
		m_commandBuffer->BindPipeline(m_currentRenderPipeline);

		SetupPipelineData();
	}

	void RenderContext::SetPipelineState(IntRef<RHI::Shader> computeShader)
	{
		m_currentComputePipeline = CreateComputePipeline(computeShader);
		m_commandBuffer->BindPipeline(m_currentComputePipeline);

		SetupPipelineData();
	}

	IntRef<RHI::RenderPipeline> RenderContext::CreateRenderPipeline(const GraphicsPipelineState& pipelineState)
	{
		VerifyGraphicsPipelineState(pipelineState);
		return PipelineStateCache::GetRenderPipeline(TranslateGraphicsPipelineState(pipelineState));
	}

	IntRef<RHI::ComputePipeline> RenderContext::CreateComputePipeline(IntRef<RHI::Shader> computeShader)
	{
		return PipelineStateCache::GetComputePipeline(computeShader);
	}

	void RenderContext::BindIndexBuffer(RGBufferRef indexBuffer)
	{
		IntRef<RHI::Buffer> rhiIndexBuffer = indexBuffer->GetRHIResource()->GetRHIBuffer();
		m_commandBuffer->BindIndexBuffer(rhiIndexBuffer);
	}

	void RenderContext::BindVertexBuffers(const InlineVector<RGBufferRef, RHI::MAX_VERTEX_BUFFER_COUNT>& vertexBuffers, const uint32_t firstBinding)
	{
		RHI::VertexBufferVector rhiVertexBuffers;
		for (const RGBufferRef buffer : vertexBuffers)
		{
			auto& vertexBufferBinding = rhiVertexBuffers.emplace_back();
			vertexBufferBinding.buffer = buffer->GetRHIResource()->GetRHIBuffer();
		}

		m_commandBuffer->BindVertexBuffers(rhiVertexBuffers, firstBinding);
	}

	void RenderContext::CopyBufferRegion(RGBufferRef src, const size_t srcOffset, RGBufferRef dst, const size_t dstOffset, const size_t size)
	{
		IntRef<RHI::Buffer> rhiSrcBuffer = src->GetRHIResource()->GetRHIBuffer();
		IntRef<RHI::Buffer> rhiDstBuffer = dst->GetRHIResource()->GetRHIBuffer();

		m_commandBuffer->CopyBufferRegion(rhiSrcBuffer, srcOffset, rhiDstBuffer, dstOffset, size);
	}

	void RenderContext::CopyTexture(RGTextureRef src, RGTextureRef dst, const uint32_t width, const uint32_t height, const uint32_t depth)
	{
		IntRef<RHI::Image> rhiSrcTexture = src->GetRHIResource()->GetRHITexture();
		IntRef<RHI::Image> rhiDstTexture = dst->GetRHIResource()->GetRHITexture();
	
		VT_ENSURE_MSG(width > 0 && height > 0 && depth > 0, "Width, height and depth must be greater than zero!");
		m_commandBuffer->CopyImage(rhiSrcTexture, rhiDstTexture, width, height, depth);
	}

	void RenderContext::UnmapBuffer(RGBufferRef buffer)
	{
		buffer->GetRHIResource()->GetRHIBuffer()->Unmap();
	}

	void RenderContext::UnmapBuffer(RGUniformBufferRef buffer)
	{
		buffer->GetRHIResource()->GetRHIUniformBuffer()->Unmap();
	}

	IntRef<RHI::CommandBuffer> RenderContext::GetRHICommandBuffer()
	{
		return m_commandBuffer;
	}

	void RenderContext::BindShaderBindings()
	{
		for (const auto& shaderParameters : m_perStageShaderParameters)
		{
			m_shaderBindingMap.SetUniformBufferWithSizeAndOffset(shaderParameters.shaderStage, RHI::Globals::SHADER_GLOBALS_BINDING, shaderParameters.uniformBufferSRV->GetRHIView(), shaderParameters.size, shaderParameters.offset);
		}

		m_commandBuffer->BindShaderBindings(m_shaderBindingMap);
	}

	void RenderContext::SetupPipelineData()
	{
		m_perStageShaderParameters.clear();

		if (m_currentComputePipeline)
		{
			m_perStageShaderParameters = SetupPipelineData(m_currentComputePipeline);
			m_shaderBindingMap = RHI::ShaderBindingMap::InitializeFromPipeline(m_currentComputePipeline);
		}
		else
		{
			m_perStageShaderParameters = SetupPipelineData(m_currentRenderPipeline);
			m_shaderBindingMap = RHI::ShaderBindingMap::InitializeFromPipeline(m_currentRenderPipeline);
		}
	}

	InlineVector<RenderContext::PerStageShaderParameters, 8> RenderContext::SetupPipelineData(RawPtr<RHI::RenderPipeline> renderPipeline)
	{
		VT_PROFILE_FUNCTION();

		ArrayView<RHI::ShaderParameterMap> shaderParameterMaps = renderPipeline->GetShaderParameterMaps();

		InlineVector<RenderContext::PerStageShaderParameters, 8> result;

		for (const auto& parameterMap : shaderParameterMaps)
		{
			if (parameterMap.GetShaderParametersSize() > 0)
			{
				auto& perStageShaderParameters = result.emplace_back();
				perStageShaderParameters.shaderStage = parameterMap.GetShaderStage();
				perStageShaderParameters.uniformBufferSRV = m_shaderParameterUniformBuffer.GetSRV();
				perStageShaderParameters.size = parameterMap.GetShaderParametersSize();
				perStageShaderParameters.offset = m_shaderParameterUniformBuffer.Allocate(parameterMap.GetShaderParametersSize());
				perStageShaderParameters.mappedPtr = m_shaderParameterUniformBuffer.GetMappedPointer() + perStageShaderParameters.offset;
			}
		}

		return result;
	}

	InlineVector<RenderContext::PerStageShaderParameters, 8> RenderContext::SetupPipelineData(RawPtr<RHI::ComputePipeline> computePipeline)
	{
		const RHI::ShaderParameterMap& shaderParameterMap = computePipeline->GetShaderParameterMap();

		InlineVector<RenderContext::PerStageShaderParameters, 8> result;

		if (shaderParameterMap.GetShaderParametersSize() > 0)
		{
			auto& perStageShaderParameters = result.emplace_back();
			perStageShaderParameters.shaderStage = shaderParameterMap.GetShaderStage();
			perStageShaderParameters.uniformBufferSRV = m_shaderParameterUniformBuffer.GetSRV();
			perStageShaderParameters.size = shaderParameterMap.GetShaderParametersSize();
			perStageShaderParameters.offset = m_shaderParameterUniformBuffer.Allocate(shaderParameterMap.GetShaderParametersSize());
			perStageShaderParameters.mappedPtr = m_shaderParameterUniformBuffer.GetMappedPointer() + perStageShaderParameters.offset;
		}

		return result;
	}

	void RenderContext::SetBufferSRVParameter(RGBufferSRVRef bufferSRV, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterDesc.GetParameterNameHash());
		if (resourceBinding)
		{
			VT_ENSURE_MSG(bufferSRV, "Buffer SRV must not be null!");
			
			IntRef<RHI::BufferView> rhiView = bufferSRV->GetRHIView();

			if (RHI::RHICanUseBindless())
			{
				SetBindlessResourceParameter(rhiView->GetSRVBindlessIndex(), resourceBinding, shaderParameterMap);
			}
			else
			{
				const bool isTexelBufferView = rhiView->IsTexelBufferView();

				if (isTexelBufferView)
				{
					m_shaderBindingMap.SetTexelBufferSRV(shaderParameterMap.GetShaderStage(), resourceBinding->binding, rhiView);
				}
				else
				{
					m_shaderBindingMap.SetStructuredBufferSRV(shaderParameterMap.GetShaderStage(), resourceBinding->binding, rhiView);
				}
			}
		}
	}

	void RenderContext::SetBufferUAVParameter(RGBufferUAVRef bufferUAV, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterDesc.GetParameterNameHash());
		if (resourceBinding)
		{
			VT_ENSURE_MSG(bufferUAV, "Buffer SRV must not be null!");

			IntRef<RHI::BufferView> rhiView = bufferUAV->GetRHIView();
			
			if (RHI::RHICanUseBindless())
			{
				SetBindlessResourceParameter(rhiView->GetUAVBindlessIndex(), resourceBinding, shaderParameterMap);
			}
			else
			{
				const bool isTexelBufferView = rhiView->IsTexelBufferView();

				if (isTexelBufferView)
				{
					m_shaderBindingMap.SetTexelBufferUAV(shaderParameterMap.GetShaderStage(), resourceBinding->binding, rhiView);
				}
				else
				{
					m_shaderBindingMap.SetStructuredBufferUAV(shaderParameterMap.GetShaderStage(), resourceBinding->binding, rhiView);
				}
			}
		}
	}

	void RenderContext::SetTextureSRVParameter(RGTextureSRVRef textureSRV, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterDesc.GetParameterNameHash());
		if (resourceBinding)
		{
			VT_ENSURE_MSG(textureSRV, "Texture SRV must not be null!");

			IntRef<RHI::ImageView> rhiView = textureSRV->GetRHIView();

			if (RHI::RHICanUseBindless())
			{
				SetBindlessResourceParameter(rhiView->GetSRVBindlessIndex(), resourceBinding, shaderParameterMap);
			}
			else
			{
				m_shaderBindingMap.SetTextureSRV(shaderParameterMap.GetShaderStage(), resourceBinding->binding, rhiView);
			}
		}
	}

	void RenderContext::SetTextureUAVParameter(RGTextureUAVRef textureUAV, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterDesc.GetParameterNameHash());
		if (resourceBinding)
		{
			VT_ENSURE_MSG(textureUAV, "Texture UAV must not be null!");

			IntRef<RHI::ImageView> rhiView = textureUAV->GetRHIView();

			if (RHI::RHICanUseBindless())
			{
				SetBindlessResourceParameter(rhiView->GetUAVBindlessIndex(), resourceBinding, shaderParameterMap);
			}
			else
			{
				m_shaderBindingMap.SetTextureUAV(shaderParameterMap.GetShaderStage(), resourceBinding->binding, textureUAV->GetRHIView());
			}
		}
	}

	void RenderContext::SetUniformBufferParameter(RGUniformBufferRef uniformBuffer, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterDesc.GetParameterNameHash());
		if (resourceBinding && uniformBuffer)
		{
			m_shaderBindingMap.SetUniformBuffer(shaderParameterMap.GetShaderStage(), resourceBinding->binding, uniformBuffer->GetRHIResource()->GetOrCreateView({}));
		}
	}

	void RenderContext::SetSamplerParameter(IntRef<RHI::SamplerState> sampler, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterDesc.GetParameterNameHash());
		if (resourceBinding)
		{
			if (RHI::RHICanUseBindless())
			{
				SetBindlessResourceParameter(sampler->GetBindlessIndex(), resourceBinding, shaderParameterMap);
			}
			else
			{
				m_shaderBindingMap.SetSampler(shaderParameterMap.GetShaderStage(), resourceBinding->binding, sampler);
			}
		}
	}

	void RenderContext::SetAccelerationStructureParameter(IntRef<RHI::AccelerationStructure> accelerationStructure, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterDesc.GetParameterNameHash());
		if (resourceBinding)
		{
			if (RHI::RHICanUseBindless())
			{
				SetBindlessResourceParameter(accelerationStructure->GetBindlessIndex(), resourceBinding, shaderParameterMap);
			}
			else
			{
				m_shaderBindingMap.SetAccelerationStructure(shaderParameterMap.GetShaderStage(), resourceBinding->binding, accelerationStructure);
			}
		}
	}

	void RenderContext::SetResourceTableParameter(IntRef<RHI::ResourceTable> resourceTable, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		m_shaderBindingMap.SetResourceTable(resourceTable);
	}

	void RenderContext::SetShaderParameter(const void* data, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		const RHI::ShaderUniform* shaderParameter = shaderParameterMap.GetParameterFromName(parameterDesc.GetParameterNameHash());
		if (shaderParameter)
		{
			VT_ENSURE(shaderParameter->size == parameterDesc.GetSize());

			for (const auto& perStageParameters : m_perStageShaderParameters)
			{
				if (perStageParameters.shaderStage == shaderParameterMap.GetShaderStage())
				{
					memcpy(perStageParameters.mappedPtr + shaderParameter->offset, data, parameterDesc.GetSize());
					break;
				}
			}
		}
	}

	void RenderContext::SetBindlessResourceParameter(RHI::BindlessIndex index, const RHI::ShaderResourceBinding* resourceBinding, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		VT_ENSURE(index.IsValid());

		const RHI::ShaderUniform* shaderParameter = shaderParameterMap.GetParameterFromName(resourceBinding->bindlessHash);
		if (shaderParameter)
		{
			VT_ENSURE(shaderParameter->size == sizeof(uint32_t));

			for (const auto& perStageParameters : m_perStageShaderParameters)
			{
				if (perStageParameters.shaderStage == shaderParameterMap.GetShaderStage())
				{
					uint32_t tempIndex = index.Get();
					memcpy(perStageParameters.mappedPtr + shaderParameter->offset, &tempIndex, sizeof(tempIndex));
					break;
				}
			}
		}
	}

	void RenderContext::CollectBufferSRVParameter(RGBufferSRVRef bufferSRV, const RenderGraphParameterDesc& parameterDesc, BatchedShaderParameters& batchedShaderParameters)
	{
		const RHI::ShaderResourceType resourceType = bufferSRV->IsTexelBufferSRV() ? RHI::ShaderResourceType::TexelBuffer : RHI::ShaderResourceType::StructuredBuffer;
		batchedShaderParameters.AddBufferParameter(parameterDesc.GetParameterNameHash(), resourceType, bufferSRV->GetRHIView());
	}

	void RenderContext::CollectBufferUAVParameter(RGBufferUAVRef bufferUAV, const RenderGraphParameterDesc& parameterDesc, BatchedShaderParameters& batchedShaderParameters)
	{
		const RHI::ShaderResourceType resourceType = bufferUAV->IsTexelBufferUAV() ? RHI::ShaderResourceType::TexelBuffer : RHI::ShaderResourceType::StructuredBuffer;
		batchedShaderParameters.AddBufferParameter(parameterDesc.GetParameterNameHash(), resourceType, bufferUAV->GetRHIView());
	}

	void RenderContext::CollectTextureSRVParameter(RGTextureSRVRef textureSRV, const RenderGraphParameterDesc& parameterDesc, BatchedShaderParameters& batchedShaderParameters)
	{
		batchedShaderParameters.AddTextureParameter(parameterDesc.GetParameterNameHash(), RHI::ShaderResourceType::Texture, textureSRV->GetRHIView());
	}

	void RenderContext::CollectTextureUAVParameter(RGTextureUAVRef textureUAV, const RenderGraphParameterDesc& parameterDesc, BatchedShaderParameters& batchedShaderParameters)
	{
		batchedShaderParameters.AddTextureParameter(parameterDesc.GetParameterNameHash(), RHI::ShaderResourceType::Texture, textureUAV->GetRHIView());
	}

	void RenderContext::CollectSamplerParameter(IntRef<RHI::SamplerState> sampler, const RenderGraphParameterDesc& parameterDesc, BatchedShaderParameters& batchedShaderParameters)
	{
		batchedShaderParameters.AddSamplerParameter(parameterDesc.GetParameterNameHash(), RHI::ShaderResourceType::Sampler, sampler);
	}

	void RenderContext::CollectUniformBufferParameter(RGUniformBufferRef uniformBuffer, const RenderGraphParameterDesc& parameterDesc, BatchedShaderParameters& batchedShaderParameters)
	{
		if (uniformBuffer)
		{
			IntRef<RHI::BufferView> bufferView = uniformBuffer->GetRHIResource()->GetOrCreateView({});
			batchedShaderParameters.AddBufferParameter(parameterDesc.GetParameterNameHash(), RHI::ShaderResourceType::UniformBuffer, bufferView);
		}
	}

	void RenderContext::CollectShaderParameter(const void* data, const RenderGraphParameterDesc& parameterDesc, BatchedShaderParameters& batchedShaderParameters)
	{
		batchedShaderParameters.AddShaderParameter(parameterDesc.GetParameterNameHash(), data, parameterDesc.GetSize());
	}

	void* RenderContext::MapInternal(RGBufferRef buffer)
	{
		IntRef<RHI::Buffer> rhiBuffer = buffer->GetRHIResource()->GetRHIBuffer();
		return rhiBuffer->Map<void>();
	}

	void* RenderContext::MapInternal(RGUniformBufferRef buffer)
	{
		return buffer->GetRHIResource()->GetRHIUniformBuffer()->Map<void>();
	}

	RHI::RenderPipelineCreateInfo RenderContext::TranslateGraphicsPipelineState(const GraphicsPipelineState& pipelineState)
	{
		VT_PROFILE_FUNCTION();

		RHI::RenderPipelineCreateInfo pipelineCreateInfo;
		pipelineCreateInfo.shaders = pipelineState.shaders;
		pipelineCreateInfo.attachmentBlendStates = pipelineState.attachmentBlendStates;
		pipelineCreateInfo.topology = pipelineState.topology;
		pipelineCreateInfo.cullMode = pipelineState.cullMode;
		pipelineCreateInfo.fillMode = pipelineState.fillMode;
		pipelineCreateInfo.depthMode = pipelineState.depthMode;
		pipelineCreateInfo.depthCompareOperator = pipelineState.depthCompareOperator;
		pipelineCreateInfo.enablePrimitiveRestart = pipelineState.enablePrimitiveRestart;
		pipelineCreateInfo.enableDepthClamp = pipelineState.enableDepthClamp;
		pipelineCreateInfo.depthBiasConstantFactor = pipelineState.depthBiasConstantFactor;
		pipelineCreateInfo.depthBiasClamp = pipelineState.depthBiasClamp;
		pipelineCreateInfo.depthBiasSlopeFactor = pipelineState.depthBiasSlopeFactor;

		const ShaderParameterRenderTargetDecl& depthDecl = pipelineState.renderTargets.depthTarget;

		if (depthDecl.texture != nullptr)
		{
			pipelineCreateInfo.depthAttachmentFormat = depthDecl.texture->GetDesc().format;
		}

		pipelineCreateInfo.colorAttachmentFormats.reserve(RHI::MAX_COLOR_ATTACHMENT_COUNT);
		for (uint32_t i = 0; i < RHI::MAX_COLOR_ATTACHMENT_COUNT; ++i)
		{
			const ShaderParameterRenderTargetDecl& rtDecl = pipelineState.renderTargets.renderTargets[i];

			if (rtDecl.texture != nullptr)
			{
				pipelineCreateInfo.colorAttachmentFormats.emplace_back(rtDecl.texture->GetDesc().format);
			}
		}

		return pipelineCreateInfo;
	}

	void RenderContext::VerifyGraphicsPipelineState(const GraphicsPipelineState& pipelineState) const
	{
		const ShaderParameterRenderTargetDecl& depthDecl = pipelineState.renderTargets.depthTarget;

		uint32_t numRenderTargets = depthDecl.texture != nullptr;
		for (uint32_t i = 0; i < RHI::MAX_COLOR_ATTACHMENT_COUNT; ++i)
		{
			const ShaderParameterRenderTargetDecl& rtDecl = pipelineState.renderTargets.renderTargets[i];
			numRenderTargets += rtDecl.texture != nullptr;
		}

		VT_ENSURE_MSG(numRenderTargets > 0, "There must always be at least 1 render target bound!");
	}
}
