#include "rcpch.h"
#include "RenderCore/RenderGraph/RenderContext.h"

#include "RenderCore/RenderGraph/RenderGraph.h"
#include "RenderCore/RenderGraph/RenderGraphCommon.h"
#include "RenderCore/Shader/BatchedShaderParameters.h"

#include <RHIModule/Buffers/UniformBuffer.h>
#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Images/ImageView.h>
#include <RHIModule/Globals.h>

namespace Volt
{
	RenderContext::RenderContext(RenderGraph& renderGraph, RenderGraphPassRef currentPass, RefPtr<RHI::CommandBuffer> commandBuffer, RenderGraphShaderParameterUniformBuffer& shaderParameterUniformBuffer)
		: m_renderGraph(renderGraph), m_currentPass(currentPass), m_commandBuffer(commandBuffer), m_shaderParameterUniformBuffer(shaderParameterUniformBuffer)
	{

	}

	void RenderContext::Flush(RefPtr<RHI::Fence> fence)
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
			if (rtBindings.renderTargets[i] != nullptr)
			{
				RefPtr<RHI::ImageView> view = rtBindings.renderTargets[i]->GetRHIResource()->GetOrCreateView({});

				RHI::AttachmentInfo& attachment = colorAttachments.emplace_back();
				attachment.clearMode = RHI::ClearMode::Clear;
				attachment.clearColor = { 0.f, 0.f, 0.f, 0.f };
				attachment.view = view;
			}
		}

		if (rtBindings.depthTarget != nullptr)
		{
			depthAttachment.clearMode = RHI::ClearMode::Clear;
			depthAttachment.clearColor = { 0.f };
			depthAttachment.view = rtBindings.depthTarget->GetRHIResource()->GetOrCreateView({});
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
			outDeclaration.colorAttachmentFormats[i] = m_activeRenderingInfo.renderingInfo.colorAttachments[i].view->GetImage()->GetFormat();
		}

		outDeclaration.depthAttachmentFormat = m_activeRenderingInfo.renderingInfo.depthAttachmentInfo.view ? m_activeRenderingInfo.renderingInfo.depthAttachmentInfo.view->GetImage()->GetFormat() : RHI::PixelFormat::UNDEFINED;
	}

	void RenderContext::DispatchMeshTasks(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ)
	{
		BindShaderBindings();

		m_commandBuffer->DispatchMeshTasks(groupCountX, groupCountY, groupCountZ);
	}

	void RenderContext::DispatchMeshTasksIndirect(RGBufferRef commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride)
	{
		BindShaderBindings();

		RefPtr<RHI::StorageBuffer> rhiCommandsBuffer = commandsBuffer->GetRHIResource()->GetRHIBuffer();
		m_commandBuffer->DispatchMeshTasksIndirect(rhiCommandsBuffer, offset, drawCount, stride);
	}

	void RenderContext::DispatchMeshTasksIndirectCount(RGBufferRef commandsBuffer, const size_t offset, RGBufferRef countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride)
	{
		BindShaderBindings();

		RefPtr<RHI::StorageBuffer> rhiCommandsBuffer = commandsBuffer->GetRHIResource()->GetRHIBuffer();
		RefPtr<RHI::StorageBuffer> rhiCountBuffer = countBuffer->GetRHIResource()->GetRHIBuffer();
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

		RefPtr<RHI::StorageBuffer> rhiCommandsBuffer = commandsBuffer->GetRHIResource()->GetRHIBuffer();
		m_commandBuffer->DispatchIndirect(rhiCommandsBuffer, offset);
	}

	void RenderContext::DrawIndirectCount(RGBufferRef commandsBuffer, const size_t offset, RGBufferRef countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride)
	{
		BindShaderBindings();

		RefPtr<RHI::StorageBuffer> rhiCommandsBuffer = commandsBuffer->GetRHIResource()->GetRHIBuffer();
		RefPtr<RHI::StorageBuffer> rhiCountBuffer = countBuffer->GetRHIResource()->GetRHIBuffer();
		m_commandBuffer->DispatchMeshTasksIndirectCount(rhiCommandsBuffer, offset, rhiCountBuffer, countBufferOffset, maxDrawCount, stride);
	}

	void RenderContext::DrawIndexedIndirect(RGBufferRef commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride)
	{
		BindShaderBindings();

		RefPtr<RHI::StorageBuffer> rhiCommandsBuffer = commandsBuffer->GetRHIResource()->GetRHIBuffer();
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
		m_commandBuffer->ClearImageView(textureUAV->GetRHIView(), std::array<uint32_t, 4>{ clearValues[0], clearValues[1], clearValues[2], clearValues[3] });
	}

	void RenderContext::ClearUAV(RGTextureUAVRef textureUAV, const glm::vec4& clearValues)
	{
		m_commandBuffer->ClearImageView(textureUAV->GetRHIView(), std::array<float, 4>{ clearValues[0], clearValues[1], clearValues[2], clearValues[3] });
	}

	void RenderContext::ClearUAV(RGBufferUAVRef bufferUAV, const uint32_t clearValue)
	{
		m_commandBuffer->ClearBufferView(bufferUAV->GetRHIView(), clearValue);
	}

	void RenderContext::ClearUAV(RGBufferUAVRef bufferUAV, const float clearValue)
	{
		m_commandBuffer->ClearBufferView(bufferUAV->GetRHIView(), clearValue);
	}

	void RenderContext::BindPipeline(RefPtr<RHI::RenderPipeline> pipeline)
	{
		m_currentRenderPipeline = pipeline;
		m_commandBuffer->BindPipeline(pipeline);

		SetupPipelineData();
	}

	void RenderContext::BindPipeline(RefPtr<RHI::ComputePipeline> pipeline)
	{
		m_currentComputePipeline = pipeline;
		m_commandBuffer->BindPipeline(pipeline);

		SetupPipelineData();
	}

	void RenderContext::BindIndexBuffer(RGBufferRef indexBuffer)
	{
		RefPtr<RHI::StorageBuffer> rhiIndexBuffer = indexBuffer->GetRHIResource()->GetRHIBuffer();
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
		RefPtr<RHI::StorageBuffer> rhiSrcBuffer = src->GetRHIResource()->GetRHIBuffer();
		RefPtr<RHI::StorageBuffer> rhiDstBuffer = dst->GetRHIResource()->GetRHIBuffer();

		m_commandBuffer->CopyBufferRegion(rhiSrcBuffer->GetAllocation(), srcOffset, rhiDstBuffer->GetAllocation(), dstOffset, size);
	}

	void RenderContext::CopyTexture(RGTextureRef src, RGTextureRef dst, const uint32_t width, const uint32_t height, const uint32_t depth)
	{
		RefPtr<RHI::Image> rhiSrcTexture = src->GetRHIResource()->GetRHITexture();
		RefPtr<RHI::Image> rhiDstTexture = dst->GetRHIResource()->GetRHITexture();
	
		VT_ENSURE_MSG(width > 0 && height > 0 && depth > 0, "Width, height and depth must be greater than zero!");
		m_commandBuffer->CopyImage(rhiSrcTexture, rhiDstTexture, width, height, depth);
	}

	void RenderContext::UnmapBuffer(RGBufferUAVRef buffer)
	{
		RGBufferRef bufferResource = reinterpret_cast<RGBufferRef>(buffer->GetResource());
		RefPtr<RHI::StorageBuffer> rhiBuffer = bufferResource->GetRHIResource()->GetRHIBuffer();
		rhiBuffer->Unmap();
	}

	void RenderContext::UnmapBuffer(RGUniformBufferRef buffer)
	{
		buffer->GetRHIResource()->GetRHIUniformBuffer()->Unmap();
	}

	RefPtr<RHI::CommandBuffer> RenderContext::GetRHICommandBuffer()
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
			
			RefPtr<RHI::BufferView> rhiView = bufferSRV->GetRHIView();
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

	void RenderContext::SetBufferUAVParameter(RGBufferUAVRef bufferUAV, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterDesc.GetParameterNameHash());
		if (resourceBinding)
		{
			VT_ENSURE_MSG(bufferUAV, "Buffer SRV must not be null!");

			RefPtr<RHI::BufferView> rhiView = bufferUAV->GetRHIView();
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

	void RenderContext::SetTextureSRVParameter(RGTextureSRVRef textureSRV, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterDesc.GetParameterNameHash());
		if (resourceBinding)
		{
			VT_ENSURE_MSG(textureSRV, "Texture SRV must not be null!");
			m_shaderBindingMap.SetTextureSRV(shaderParameterMap.GetShaderStage(), resourceBinding->binding, textureSRV->GetRHIView());
		}
	}

	void RenderContext::SetTextureUAVParameter(RGTextureUAVRef textureUAV, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterDesc.GetParameterNameHash());
		if (resourceBinding)
		{
			VT_ENSURE_MSG(textureUAV, "Texture UAV must not be null!");
			m_shaderBindingMap.SetTextureUAV(shaderParameterMap.GetShaderStage(), resourceBinding->binding, textureUAV->GetRHIView());
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

	void RenderContext::SetSamplerParameter(RefPtr<RHI::SamplerState> sampler, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterDesc.GetParameterNameHash());
		if (resourceBinding)
		{
			m_shaderBindingMap.SetSampler(shaderParameterMap.GetShaderStage(), resourceBinding->binding, sampler);
		}
	}

	void RenderContext::SetAccelerationStructureParameter(RefPtr<RHI::AccelerationStructure> accelerationStructure, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterDesc.GetParameterNameHash());
		if (resourceBinding)
		{
			m_shaderBindingMap.SetAccelerationStructure(shaderParameterMap.GetShaderStage(), resourceBinding->binding, accelerationStructure);
		}
	}

	void RenderContext::SetRayTracingResourceTableParameter(RefPtr<RHI::RayTracingResourceTable> rayTracingResourceTable, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		m_shaderBindingMap.SetRayTracingResourceTable(rayTracingResourceTable);
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

	void RenderContext::CollectSamplerParameter(RefPtr<RHI::SamplerState> sampler, const RenderGraphParameterDesc& parameterDesc, BatchedShaderParameters& batchedShaderParameters)
	{
		batchedShaderParameters.AddSamplerParameter(parameterDesc.GetParameterNameHash(), RHI::ShaderResourceType::Sampler, sampler);
	}

	void RenderContext::CollectUniformBufferParameter(RGUniformBufferRef uniformBuffer, const RenderGraphParameterDesc& parameterDesc, BatchedShaderParameters& batchedShaderParameters)
	{
		if (uniformBuffer)
		{
			RefPtr<RHI::BufferView> bufferView = uniformBuffer->GetRHIResource()->GetOrCreateView({});
			batchedShaderParameters.AddBufferParameter(parameterDesc.GetParameterNameHash(), RHI::ShaderResourceType::UniformBuffer, bufferView);
		}
	}

	void RenderContext::CollectShaderParameter(const void* data, const RenderGraphParameterDesc& parameterDesc, BatchedShaderParameters& batchedShaderParameters)
	{
		batchedShaderParameters.AddShaderParameter(parameterDesc.GetParameterNameHash(), data, parameterDesc.GetSize());
	}

	void* RenderContext::MapInternal(RGBufferUAVRef buffer)
	{
		RGBufferRef bufferResource = reinterpret_cast<RGBufferRef>(buffer->GetResource());

		RefPtr<RHI::StorageBuffer> rhiBuffer = bufferResource->GetRHIResource()->GetRHIBuffer();
		return rhiBuffer->Map<void>();
	}

	void* RenderContext::MapInternal(RGUniformBufferRef buffer)
	{
		return buffer->GetRHIResource()->GetRHIUniformBuffer()->Map<void>();
	}
}
