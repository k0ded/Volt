#include "rcpch.h"
#include "RenderContext.h"

#include "RenderCore/RenderGraph/RenderGraph.h"
#include "RenderCore/RenderGraph/RenderGraphCommon.h"
#include "RenderCore/DescriptorTableCache.h"
#include "RenderCore/Shader/BatchedShaderParameters.h"

#include <RHIModule/Buffers/UniformBuffer.h>
#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Images/ImageView.h>
#include <RHIModule/Globals.h>

namespace Volt
{

	RenderContext::RenderContext(RenderGraph& renderGraph, RenderGraphPass* currentPass, RefPtr<RHI::CommandBuffer> commandBuffer, RenderGraphShaderParameterUniformBuffer& shaderParameterUniformBuffer)
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
	}

	void RenderContext::EndRendering()
	{
		VT_PROFILE_FUNCTION();

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

		StackVector<RHI::AttachmentInfo, RHI::MAX_COLOR_ATTACHMENT_COUNT> colorAttachments;
		RHI::AttachmentInfo depthAttachment{};

		for (size_t i = 0; i < RHI::MAX_COLOR_ATTACHMENT_COUNT; ++i)
		{
			if (rtBindings.renderTargets[i] != nullptr)
			{
				RefPtr<RHI::ImageView> view = rtBindings.renderTargets[i]->GetRHIResource()->GetOrCreateView({});

				RHI::AttachmentInfo attachment{};
				attachment.clearMode = RHI::ClearMode::Clear;
				attachment.clearColor = { 0.f, 0.f, 0.f, 0.f };
				attachment.view = view;

				colorAttachments.Push(attachment);
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

	void RenderContext::DispatchMeshTasks(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ)
	{
		BindDescriptorTable();

		m_commandBuffer->DispatchMeshTasks(groupCountX, groupCountY, groupCountZ);
	}

	void RenderContext::DispatchMeshTasksIndirect(RGBufferRef commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride)
	{
		BindDescriptorTable();

		RefPtr<RHI::StorageBuffer> rhiCommandsBuffer = commandsBuffer->GetRHIResource()->GetRHIBuffer();
		m_commandBuffer->DispatchMeshTasksIndirect(rhiCommandsBuffer, offset, drawCount, stride);
	}

	void RenderContext::DispatchMeshTasksIndirectCount(RGBufferRef commandsBuffer, const size_t offset, RGBufferRef countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride)
	{
		BindDescriptorTable();

		RefPtr<RHI::StorageBuffer> rhiCommandsBuffer = commandsBuffer->GetRHIResource()->GetRHIBuffer();
		RefPtr<RHI::StorageBuffer> rhiCountBuffer = countBuffer->GetRHIResource()->GetRHIBuffer();
		m_commandBuffer->DispatchMeshTasksIndirectCount(rhiCommandsBuffer, offset, rhiCountBuffer, countBufferOffset, maxDrawCount, stride);
	}

	void RenderContext::Dispatch(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ)
	{
		BindDescriptorTable();

		m_commandBuffer->Dispatch(groupCountX, groupCountY, groupCountZ);
	}

	void RenderContext::DispatchIndirect(RGBufferRef commandsBuffer, const size_t offset)
	{
		BindDescriptorTable();

		RefPtr<RHI::StorageBuffer> rhiCommandsBuffer = commandsBuffer->GetRHIResource()->GetRHIBuffer();
		m_commandBuffer->DispatchIndirect(rhiCommandsBuffer, offset);
	}

	void RenderContext::DrawIndirectCount(RGBufferRef commandsBuffer, const size_t offset, RGBufferRef countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride)
	{
		BindDescriptorTable();

		RefPtr<RHI::StorageBuffer> rhiCommandsBuffer = commandsBuffer->GetRHIResource()->GetRHIBuffer();
		RefPtr<RHI::StorageBuffer> rhiCountBuffer = countBuffer->GetRHIResource()->GetRHIBuffer();
		m_commandBuffer->DispatchMeshTasksIndirectCount(rhiCommandsBuffer, offset, rhiCountBuffer, countBufferOffset, maxDrawCount, stride);
	}

	void RenderContext::DrawIndexedIndirect(RGBufferRef commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride)
	{
		BindDescriptorTable();

		RefPtr<RHI::StorageBuffer> rhiCommandsBuffer = commandsBuffer->GetRHIResource()->GetRHIBuffer();
		m_commandBuffer->DrawIndexedIndirect(rhiCommandsBuffer, offset, drawCount, stride);
	}

	void RenderContext::DrawIndexed(const uint32_t indexCount, const uint32_t instanceCount, const uint32_t firstIndex, const uint32_t vertexOffset, const uint32_t firstInstance)
	{
		BindDescriptorTable();

		m_commandBuffer->DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void RenderContext::Draw(const uint32_t vertexCount, const uint32_t instanceCount, const uint32_t firstVertex, const uint32_t firstInstance)
	{
		BindDescriptorTable();
	
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

		AllocatePerStageShaderParameterBuffers();

		m_descriptorTable = DescriptorTableCache::Get().GetOrCreateDescriptorTableForPipeline(pipeline);
	}

	void RenderContext::BindPipeline(RefPtr<RHI::ComputePipeline> pipeline)
	{
		m_currentComputePipeline = pipeline;
		m_commandBuffer->BindPipeline(pipeline);

		AllocatePerStageShaderParameterBuffers();

		m_descriptorTable = DescriptorTableCache::Get().GetOrCreateDescriptorTableForPipeline(pipeline);
	}

	void RenderContext::BindIndexBuffer(RGBufferRef indexBuffer)
	{
		RefPtr<RHI::StorageBuffer> rhiIndexBuffer = indexBuffer->GetRHIResource()->GetRHIBuffer();
		m_commandBuffer->BindIndexBuffer(rhiIndexBuffer);
	}

	void RenderContext::BindVertexBuffers(const StackVector<RGBufferRef, RHI::MAX_VERTEX_BUFFER_COUNT>& vertexBuffers, const uint32_t firstBinding)
	{
		RHI::VertexBufferVector rhiVertexBuffers;
		for (const RGBufferRef buffer : vertexBuffers)
		{
			rhiVertexBuffers.emplace_back() = buffer->GetRHIResource()->GetRHIBuffer();
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

	void RenderContext::BindDescriptorTable()
	{
		VT_ENSURE(m_descriptorTable);

		for (const auto& shaderParameters : m_perStageShaderParameters)
		{
			m_descriptorTable->SetBufferView(shaderParameters.uniformBufferSRV->GetRHIView(), RHI::GetDescriptorSetIndexFromShaderStage(shaderParameters.shaderStage), RHI::Globals::SHADER_GLOBALS_BINDING);
		}

		m_commandBuffer->BindDescriptorTable(m_descriptorTable);
	}

	void RenderContext::AllocatePerStageShaderParameterBuffers()
	{
		m_perStageShaderParameters.clear();

		if (m_currentComputePipeline)
		{
			m_perStageShaderParameters = AllocatePerStageShaderParameterBuffers(m_currentComputePipeline);
		}
		else
		{
			m_perStageShaderParameters = AllocatePerStageShaderParameterBuffers(m_currentRenderPipeline);
		}
	}

	InlineVector<RenderContext::PerStageShaderParameters, 8> RenderContext::AllocatePerStageShaderParameterBuffers(RawPtr<RHI::RenderPipeline> renderPipeline)
	{
		const Vector<RHI::ShaderParameterMap>& shaderParameterMaps = renderPipeline->GetShaderParameterMaps();

		InlineVector<RenderContext::PerStageShaderParameters, 8> result;

		for (const auto& parameterMap : shaderParameterMaps)
		{
			if (parameterMap.GetShaderParametersSize() > 0)
			{
				auto& perStageShaderParameters = result.emplace_back();
				perStageShaderParameters.shaderStage = parameterMap.GetShaderStage();
				perStageShaderParameters.uniformBufferSRV = m_shaderParameterUniformBuffer.GetNext();
				perStageShaderParameters.mappedPtr = m_shaderParameterUniformBuffer.GetMappedPointer() + perStageShaderParameters.uniformBufferSRV->GetDesc().offset;
			}
		}

		return result;
	}

	InlineVector<RenderContext::PerStageShaderParameters, 8> RenderContext::AllocatePerStageShaderParameterBuffers(RawPtr<RHI::ComputePipeline> computePipeline)
	{
		const RHI::ShaderParameterMap& shaderParameterMap = computePipeline->GetShaderParameterMap();

		InlineVector<RenderContext::PerStageShaderParameters, 8> result;

		if (shaderParameterMap.GetShaderParametersSize() > 0)
		{
			auto& perStageShaderParameters = result.emplace_back();
			perStageShaderParameters.shaderStage = shaderParameterMap.GetShaderStage();
			perStageShaderParameters.uniformBufferSRV = m_shaderParameterUniformBuffer.GetNext();
			perStageShaderParameters.mappedPtr = m_shaderParameterUniformBuffer.GetMappedPointer() + perStageShaderParameters.uniformBufferSRV->GetDesc().offset;
		}

		return result;
	}

	void RenderContext::SetBufferSRVParameter(RGBufferSRVRef bufferSRV, const ShaderParameterMetadata& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		VT_ENSURE(m_descriptorTable);

		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterMetadata.hashedName);
		if (resourceBinding)
		{
			VT_ENSURE_MSG(bufferSRV, "Buffer SRV must not be null!");
			m_descriptorTable->SetBufferView(bufferSRV->GetRHIView(), resourceBinding->set, resourceBinding->binding);
		}
	}

	void RenderContext::SetBufferUAVParameter(RGBufferUAVRef bufferUAV, const ShaderParameterMetadata& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		VT_ENSURE(m_descriptorTable);

		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterMetadata.hashedName);
		if (resourceBinding)
		{
			VT_ENSURE_MSG(bufferUAV, "Buffer UAV must not be null!");
			m_descriptorTable->SetBufferView(bufferUAV->GetRHIView(), resourceBinding->set, resourceBinding->binding);
		}
	}

	void RenderContext::SetTextureSRVParameter(RGTextureSRVRef textureSRV, const ShaderParameterMetadata& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		VT_ENSURE(m_descriptorTable);
	
		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterMetadata.hashedName);
		if (resourceBinding)
		{
			VT_ENSURE_MSG(textureSRV, "Texture SRV must not be null!");

			m_descriptorTable->SetImageView(textureSRV->GetRHIView(), resourceBinding->set, resourceBinding->binding);
		}
	}

	void RenderContext::SetTextureUAVParameter(RGTextureUAVRef textureUAV, const ShaderParameterMetadata& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		VT_ENSURE(m_descriptorTable);

		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterMetadata.hashedName);
		if (resourceBinding)
		{
			VT_ENSURE_MSG(textureUAV, "Texture UAV must not be null!");

			m_descriptorTable->SetImageView(textureUAV->GetRHIView(), resourceBinding->set, resourceBinding->binding);
		}
	}

	void RenderContext::SetUniformBufferParameter(RGUniformBufferRef uniformBuffer, const ShaderParameterMetadata& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		VT_ENSURE(m_descriptorTable);

		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterMetadata.hashedName);
		if (resourceBinding && uniformBuffer)
		{
			VT_ENSURE_MSG(uniformBuffer, "Uniform buffer must not be null!");

			RefPtr<RHI::BufferView> bufferView = uniformBuffer->GetRHIResource()->GetOrCreateView({});
			m_descriptorTable->SetBufferView(bufferView, resourceBinding->set, resourceBinding->binding);
		}
	}

	void RenderContext::SetSamplerParameter(RefPtr<RHI::SamplerState> sampler, const ShaderParameterMetadata& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		VT_ENSURE(m_descriptorTable);

		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterMetadata.hashedName);
		if (resourceBinding)
		{
			m_descriptorTable->SetSamplerState(sampler, resourceBinding->set, resourceBinding->binding);
		}
	}

	void RenderContext::SetAccelerationStructureParameter(RefPtr<RHI::AccelerationStructure> accelerationStructure, const ShaderParameterMetadata& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		VT_ENSURE(m_descriptorTable);

		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterMetadata.hashedName);
		if (resourceBinding && accelerationStructure)
		{
			m_descriptorTable->SetAccelerationStructure(accelerationStructure, resourceBinding->set, resourceBinding->binding);
		}
	}

	void RenderContext::SetRayTracingResourceTableParameter(RefPtr<RHI::RayTracingResourceTable> rayTracingResourceTable, const ShaderParameterMetadata& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		m_descriptorTable->SetRayTracingResourceTable(rayTracingResourceTable);
	}

	void RenderContext::SetShaderParameter(const void* data, const ShaderParameterMetadata& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		const RHI::ShaderUniform* shaderParameter = shaderParameterMap.GetParameterFromName(parameterMetadata.hashedName);
		if (shaderParameter)
		{
			VT_ENSURE(shaderParameter->size == parameterMetadata.structSize);

			for (const auto& perStageParameters : m_perStageShaderParameters)
			{
				if (perStageParameters.shaderStage == shaderParameterMap.GetShaderStage())
				{
					memcpy(perStageParameters.mappedPtr + shaderParameter->offset, data, parameterMetadata.structSize);
					break;
				}
			}
		}
	}

	void RenderContext::CollectBufferSRVParameter(RGBufferSRVRef bufferSRV, const ShaderParameterMetadata& parameterMetadata, BatchedShaderParameters& batchedShaderParameters)
	{
		batchedShaderParameters.AddBufferParameter(parameterMetadata.hashedName, RHI::ShaderResourceType::StructuredBuffer, bufferSRV->GetRHIView());
	}

	void RenderContext::CollectBufferUAVParameter(RGBufferUAVRef bufferUAV, const ShaderParameterMetadata& parameterMetadata, BatchedShaderParameters& batchedShaderParameters)
	{
		batchedShaderParameters.AddBufferParameter(parameterMetadata.hashedName, RHI::ShaderResourceType::StructuredBuffer, bufferUAV->GetRHIView());
	}

	void RenderContext::CollectTextureSRVParameter(RGTextureSRVRef textureSRV, const ShaderParameterMetadata& parameterMetadata, BatchedShaderParameters& batchedShaderParameters)
	{
		batchedShaderParameters.AddTextureParameter(parameterMetadata.hashedName, RHI::ShaderResourceType::Texture, textureSRV->GetRHIView());
	}

	void RenderContext::CollectTextureUAVParameter(RGTextureUAVRef textureUAV, const ShaderParameterMetadata& parameterMetadata, BatchedShaderParameters& batchedShaderParameters)
	{
		batchedShaderParameters.AddTextureParameter(parameterMetadata.hashedName, RHI::ShaderResourceType::Texture, textureUAV->GetRHIView());
	}

	void RenderContext::CollectSamplerParameter(RefPtr<RHI::SamplerState> sampler, const ShaderParameterMetadata& parameterMetadata, BatchedShaderParameters& batchedShaderParameters)
	{
		batchedShaderParameters.AddSamplerParameter(parameterMetadata.hashedName, RHI::ShaderResourceType::Sampler, sampler);
	}

	void RenderContext::CollectUniformBufferParameter(RGUniformBufferRef uniformBuffer, const ShaderParameterMetadata& parameterMetadata, BatchedShaderParameters& batchedShaderParameters)
	{
		RefPtr<RHI::BufferView> bufferView = uniformBuffer->GetRHIResource()->GetOrCreateView({});
		batchedShaderParameters.AddBufferParameter(parameterMetadata.hashedName, RHI::ShaderResourceType::UniformBuffer, bufferView);
	}

	void RenderContext::CollectShaderParameter(const void* data, const ShaderParameterMetadata& parameterMetadata, BatchedShaderParameters& batchedShaderParameters)
	{
		batchedShaderParameters.AddShaderParameter(parameterMetadata.hashedName, data, parameterMetadata.structSize);
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
