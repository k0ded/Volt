#pragma once

#include "RenderCore/Config.h"
#include "RenderCore/RenderGraph/ShaderRegistry.h"
#include "RenderCore/RenderGraph/RenderGraphPass.h"

#include <RHIModule/Pipelines/RenderPipeline.h>
#include <RHIModule/Pipelines/ComputePipeline.h>
#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Buffers/UniformBuffer.h>
#include <RHIModule/Images/SamplerState.h>
#include <RHIModule/Core/RenderingInfo.h>
#include <RHIModule/Synchronization/Fence.h>

#include <RHIModule/Descriptors/ShaderBindingMap.h>

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	class RenderGraph;
	class RenderGraphShaderParameterUniformBuffer;
	class BatchedShaderParameters;

	struct RenderingInfo
	{
		RHI::Rect2D scissor{};
		RHI::Viewport viewport{};
		RHI::RenderingInfo renderingInfo{};
	};

	struct GraphicsPipelineState
	{
		RHI::PipelineShadersVector shaders;

		Array<RHI::AttachmentBlendState, RHI::MAX_COLOR_ATTACHMENT_COUNT> attachmentBlendStates;
		ShaderParameterRenderTargetBindings renderTargets;

		float depthBiasConstantFactor = 0.f;
		float depthBiasClamp = 0.f;
		float depthBiasSlopeFactor = 0.f;

		RHI::Topology topology = RHI::Topology::TriangleList;
		RHI::CullMode cullMode = RHI::CullMode::Back;
		RHI::FillMode fillMode = RHI::FillMode::Solid;
		RHI::DepthMode depthMode = RHI::DepthMode::ReadWrite;
		RHI::CompareOperator depthCompareOperator = RHI::CompareOperator::GreaterEqual;
		bool enablePrimitiveRestart = false;
		bool enableDepthClamp = false;
	};

	class VTRC_API RenderContext
	{
	public:
		struct PerStageShaderParameters
		{
			RHI::ShaderStage shaderStage;
			uint32_t size;
			RGUniformBufferSRVRef uniformBufferSRV;
			uint64_t offset;
			uint8_t* mappedPtr;
		};

		RenderContext(RenderGraph& renderGraph, RGPassRef currentPass, RefPtr<RHI::CommandBuffer> commandBuffer, RenderGraphShaderParameterUniformBuffer& shaderParameterUniformBuffer);

		void Flush(RefPtr<RHI::Fence> fence);

		void BeginRendering(const RenderingInfo& renderingInfo);
		void EndRendering();

		const RenderingInfo CreateRenderingInfo(const uint32_t width, const uint32_t height, const ShaderParameterRenderTargetBindings& rtBindings);
		void FillRenderingAttachmentDeclaration(RHI::RenderingAttachmentDeclaration& outDeclaration) const;

		void DispatchMeshTasks(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ);
		void DispatchMeshTasksIndirect(RGBufferRef commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride);
		void DispatchMeshTasksIndirectCount(RGBufferRef commandsBuffer, const size_t offset, RGBufferRef countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride);

		void Dispatch(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ);
		void DispatchIndirect(RGBufferRef commandsBuffer, const size_t offset);

		void DrawIndirectCount(RGBufferRef commandsBuffer, const size_t offset, RGBufferRef countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride);
		void DrawIndexedIndirect(RGBufferRef commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride);
		void DrawIndexed(const uint32_t indexCount, const uint32_t instanceCount, const uint32_t firstIndex, const uint32_t vertexOffset, const uint32_t firstInstance);
		void Draw(const uint32_t vertexCount, const uint32_t instanceCount, const uint32_t firstVertex, const uint32_t firstInstance);

		void ClearUAV(RGTextureUAVRef textureUAV, const glm::uvec4& clearValues);
		void ClearUAV(RGTextureUAVRef textureUAV, const glm::vec4& clearValues);
		void ClearUAV(RGBufferUAVRef bufferUAV, const uint32_t clearValue);
		void ClearUAV(RGBufferUAVRef bufferUAV, const float clearValue);

		void SetPipelineState(const GraphicsPipelineState& pipelineState);
		void SetPipelineState(RefPtr<RHI::Shader> computeShader);

		RefPtr<RHI::RenderPipeline> CreateRenderPipeline(const GraphicsPipelineState& pipelineState);
		RefPtr<RHI::ComputePipeline> CreateComputePipeline(RefPtr<RHI::Shader> computeShader);

		void BindIndexBuffer(RGBufferRef indexBuffer);
		void BindVertexBuffers(const InlineVector<RGBufferRef, RHI::MAX_VERTEX_BUFFER_COUNT>& vertexBuffers, const uint32_t firstBinding);

		void CopyBufferRegion(RGBufferRef src, const size_t srcOffset, RGBufferRef dst, const size_t dstOffset, const size_t size);
		void CopyTexture(RGTextureRef src, RGTextureRef dst, const uint32_t width, const uint32_t height, const uint32_t depth);

		template<typename T> T* MapBuffer(RGBufferUAVRef buffer);
		template<typename T> T* MapBuffer(RGUniformBufferRef buffer);

		void UnmapBuffer(RGBufferUAVRef buffer);
		void UnmapBuffer(RGUniformBufferRef buffer);

		template<typename ShaderType> void SetParameters(RefPtr<RHI::Shader> shader, const typename ShaderType::Parameters* parameters);
		template<typename ParameterStruct> void CollectParameters(const ParameterStruct* parameters, BatchedShaderParameters& batchedShaderParameters);

		RefPtr<RHI::CommandBuffer> GetRHICommandBuffer();

		InlineVector<PerStageShaderParameters, 8> SetupPipelineData(RawPtr<RHI::RenderPipeline> renderPipeline);
		InlineVector<PerStageShaderParameters, 8> SetupPipelineData(RawPtr<RHI::ComputePipeline> computePipeline);

		VT_INLINE const RenderingInfo& GetActiveRenderingInfo() const { VT_ASSERT(m_isWithinRenderingScope); return m_activeRenderingInfo; }

	private:
		void BindShaderBindings();
		void SetupPipelineData();

		template<typename ParameterStruct>
		void VerifyShaderParameters(RefPtr<RHI::Shader> shader, const ParameterStruct* parameters);

		void SetBufferSRVParameter(RGBufferSRVRef bufferSRV, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap);
		void SetBufferUAVParameter(RGBufferUAVRef bufferUAV, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap);
		void SetTextureSRVParameter(RGTextureSRVRef textureSRV, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap);
		void SetTextureUAVParameter(RGTextureUAVRef textureUAV, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap);
		void SetUniformBufferParameter(RGUniformBufferRef uniformBuffer, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap);
		void SetSamplerParameter(RefPtr<RHI::SamplerState> sampler, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap);
		void SetAccelerationStructureParameter(RefPtr<RHI::AccelerationStructure> accelerationStructure, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap);
		void SetRayTracingResourceTableParameter(RefPtr<RHI::RayTracingResourceTable> rayTracingResourceTable, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap);
		void SetShaderParameter(const void* data, const RenderGraphParameterDesc& parameterDesc, const RHI::ShaderParameterMap& shaderParameterMap);

		void CollectBufferSRVParameter(RGBufferSRVRef bufferSRV, const RenderGraphParameterDesc& parameterDesc, BatchedShaderParameters& batchedShaderParameters);
		void CollectBufferUAVParameter(RGBufferUAVRef bufferUAV, const RenderGraphParameterDesc& parameterDesc, BatchedShaderParameters& batchedShaderParameters);
		void CollectTextureSRVParameter(RGTextureSRVRef textureSRV, const RenderGraphParameterDesc& parameterDesc, BatchedShaderParameters& batchedShaderParameters);
		void CollectTextureUAVParameter(RGTextureUAVRef textureUAV, const RenderGraphParameterDesc& parameterDesc, BatchedShaderParameters& batchedShaderParameters);
		void CollectSamplerParameter(RefPtr<RHI::SamplerState> sampler, const RenderGraphParameterDesc& parameterDesc, BatchedShaderParameters& batchedShaderParameters);
		void CollectUniformBufferParameter(RGUniformBufferRef uniformBuffer, const RenderGraphParameterDesc& parameterDesc, BatchedShaderParameters& batchedShaderParameters);
		void CollectShaderParameter(const void* data, const RenderGraphParameterDesc& parameterDesc, BatchedShaderParameters& batchedShaderParameters);

		void* MapInternal(RGBufferUAVRef buffer);
		void* MapInternal(RGUniformBufferRef buffer);

		RHI::RenderPipelineCreateInfo TranslateGraphicsPipelineState(const GraphicsPipelineState& pipelineState);
		void VerifyGraphicsPipelineState(const GraphicsPipelineState& pipelineState) const;

		RefPtr<RHI::RenderPipeline> m_currentRenderPipeline;
		RefPtr<RHI::ComputePipeline> m_currentComputePipeline;
		RefPtr<RHI::CommandBuffer> m_commandBuffer;

		RHI::ShaderBindingMap m_shaderBindingMap;

		InlineVector<PerStageShaderParameters, 8> m_perStageShaderParameters;

		RenderGraph& m_renderGraph;
		RGPassRef m_currentPass;
		RenderGraphShaderParameterUniformBuffer& m_shaderParameterUniformBuffer;
		RenderingInfo m_activeRenderingInfo{};
		bool m_isWithinRenderingScope = false;
	};
}

#include "RenderCore/RenderGraph/RenderContext.inl"
