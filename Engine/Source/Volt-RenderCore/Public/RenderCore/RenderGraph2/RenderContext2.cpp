#include "rcpch.h"
#include "RenderContext2.h"

#include "RenderCore/RenderGraph2/RenderGraph2.h"
#include "RenderCore/RenderGraph/RenderGraphCommon.h"
#include "RenderCore/DescriptorTableCache.h"

#include <RHIModule/Buffers/UniformBuffer.h>
#include <RHIModule/Images/ImageView.h>
#include <RHIModule/Globals.h>

namespace Volt
{

	RenderContext2::RenderContext2(RenderGraph2& renderGraph, SharedRenderContext& sharedRenderContext, RenderGraphPass* currentPass, RefPtr<RHI::CommandBuffer> commandBuffer)
		: m_renderGraph(renderGraph), m_sharedRenderContext(sharedRenderContext), m_currentPass(currentPass), m_commandBuffer(commandBuffer)
	{

	}

	void RenderContext2::BeginRendering(const RenderingInfo2& renderingInfo)
	{
		VT_PROFILE_FUNCTION();

		m_commandBuffer->SetViewports({ renderingInfo.viewport });
		m_commandBuffer->SetScissors({ renderingInfo.scissor });
		m_commandBuffer->BeginRendering(renderingInfo.renderingInfo);
	}

	void RenderContext2::EndRendering()
	{
		VT_PROFILE_FUNCTION();

		m_commandBuffer->EndRendering();
	}

	const RenderingInfo2 RenderContext2::CreateRenderingInfo(const uint32_t width, const uint32_t height, const ShaderParameterRenderTargetBindings& rtBindings)
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
				RefPtr<RHI::ImageView> view = m_renderGraph.GetRHITextureRT(rtBindings.renderTargets[i]);

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
			depthAttachment.view = m_renderGraph.GetRHITextureRT(rtBindings.depthTarget);
		}

		RHI::RenderingInfo renderingInfo{};
		renderingInfo.colorAttachments = colorAttachments;
		renderingInfo.depthAttachmentInfo = depthAttachment;
		renderingInfo.renderArea = scissor;

		RenderingInfo2 result{};
		result.renderingInfo = renderingInfo;
		result.scissor = scissor;
		result.viewport = viewport;

		return result;
	}

	void RenderContext2::Dispatch(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ)
	{
		BindDescriptorTable();

		m_commandBuffer->Dispatch(groupCountX, groupCountY, groupCountZ);
	}

	void RenderContext2::Draw(const uint32_t vertexCount, const uint32_t instanceCount, const uint32_t firstVertex, const uint32_t firstInstance)
	{
		BindDescriptorTable();
	
		m_commandBuffer->Draw(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void RenderContext2::BindPipeline(RefPtr<RHI::RenderPipeline> pipeline)
	{
		m_currentRenderPipeline = pipeline;
		m_commandBuffer->BindPipeline(pipeline);

		AllocatePerStageShaderParameterBuffers();

		m_descriptorTable = DescriptorTableCache::Get().GetOrCreateDescriptorTableForPipeline(pipeline);
	}

	void RenderContext2::BindPipeline(RefPtr<RHI::ComputePipeline> pipeline)
	{
		m_currentComputePipeline = pipeline;
		m_commandBuffer->BindPipeline(pipeline);

		AllocatePerStageShaderParameterBuffers();

		m_descriptorTable = DescriptorTableCache::Get().GetOrCreateDescriptorTableForPipeline(pipeline);
	}

	void RenderContext2::BindDescriptorTable()
	{
		VT_ENSURE(m_descriptorTable);

		for (const auto& shaderParameters : m_perStageShaderParameters)
		{
			shaderParameters.uniformBuffer->Unmap();

			m_descriptorTable->SetBufferView(shaderParameters.uniformBuffer->GetView(), RHI::GetDescriptorSetIndexFromShaderStage(shaderParameters.shaderStage), RHI::Globals::SHADER_GLOBALS_BINDING);
		}

		m_commandBuffer->BindDescriptorTable2(m_descriptorTable);
	}

	void RenderContext2::AllocatePerStageShaderParameterBuffers()
	{
		m_perStageShaderParameters.clear();

		if (m_currentComputePipeline)
		{
			const RHI::ShaderParameterMap& shaderParameterMap = m_currentComputePipeline->GetShaderParameterMap();

			RGUniformBufferDesc desc{};
			desc.count = 1;
			desc.elementSize = shaderParameterMap.GetShaderParametersSize();
			desc.name = "ShaderParameters";

			RGUniformBufferRef uniformBuffer = m_renderGraph.CreateUniformBuffer(desc);
			RefPtr<RHI::UniformBuffer> rhiUniformBuffer = m_renderGraph.m_transientResourceSystem.AcquireUniformBuffer(uniformBuffer);

			auto& perStageShaderParameters = m_perStageShaderParameters.emplace_back();
			perStageShaderParameters.shaderStage = shaderParameterMap.GetShaderStage();
			perStageShaderParameters.uniformBuffer = rhiUniformBuffer;
			perStageShaderParameters.mappedPtr = rhiUniformBuffer->Map<uint8_t>();
		}
		else
		{
			const Vector<RHI::ShaderParameterMap>& shaderParameterMaps = m_currentRenderPipeline->GetShaderParameterMaps();

			RGUniformBufferDesc desc{};
			desc.count = 1;
			desc.name = "ShaderParameters";

			for (const auto& parameterMap : shaderParameterMaps)
			{
				desc.elementSize = parameterMap.GetShaderParametersSize();

				RGUniformBufferRef uniformBuffer = m_renderGraph.CreateUniformBuffer(desc);
				RefPtr<RHI::UniformBuffer> rhiUniformBuffer = m_renderGraph.m_transientResourceSystem.AcquireUniformBuffer(uniformBuffer);

				auto& perStageShaderParameters = m_perStageShaderParameters.emplace_back();
				perStageShaderParameters.shaderStage = parameterMap.GetShaderStage();
				perStageShaderParameters.uniformBuffer = rhiUniformBuffer;
				perStageShaderParameters.mappedPtr = rhiUniformBuffer->Map<uint8_t>();
			}
		}
	}

	void RenderContext2::SetBufferSRVParameter(RGBufferSRVRef bufferSRV, const ShaderParameterMetadata2& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		VT_ENSURE(m_descriptorTable);

		RefPtr<RHI::BufferView> bufferView = m_renderGraph.GetRHIBufferSRV(bufferSRV);

		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterMetadata.hashedName);
		if (resourceBinding)
		{
			m_descriptorTable->SetBufferView(bufferView, resourceBinding->set, resourceBinding->binding);
		}
	}

	void RenderContext2::SetBufferUAVParameter(RGBufferUAVRef bufferUAV, const ShaderParameterMetadata2& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		VT_ENSURE(m_descriptorTable);

		RefPtr<RHI::BufferView> bufferView = m_renderGraph.GetRHIBufferUAV(bufferUAV);

		const RHI::ShaderResourceBinding* resourceBinding = shaderParameterMap.GetResourceBindingFromName(parameterMetadata.hashedName);
		if (resourceBinding)
		{
			m_descriptorTable->SetBufferView(bufferView, resourceBinding->set, resourceBinding->binding);
		}
	}

	void RenderContext2::SetTextureSRVParameter(RGTextureSRVRef textureSRV, const ShaderParameterMetadata2& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		VT_ENSURE(m_descriptorTable);
	}

	void RenderContext2::SetTextureUAVParameter(RGTextureUAVRef textureUAV, const ShaderParameterMetadata2& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap)
	{
		VT_ENSURE(m_descriptorTable);
	}

	void RenderContext2::SetShaderParameter(const void* data, const ShaderParameterMetadata2& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap)
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
}
