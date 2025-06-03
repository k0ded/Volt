#pragma once

#include "RenderCore/Config.h"
#include "RenderCore/RenderGraph/ShaderRegistry.h"

#include <RHIModule/Pipelines/RenderPipeline.h>
#include <RHIModule/Pipelines/ComputePipeline.h>
#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Buffers/UniformBuffer.h>

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	class RenderGraph;
	class SharedRenderContext;
	class RenderGraphPass;
	class BatchedShaderParameters;

	struct RenderingInfo2
	{
		RHI::Rect2D scissor{};
		RHI::Viewport viewport{};
		RHI::RenderingInfo renderingInfo{};
	};

	class VTRC_API RenderContext
	{
	public:
		RenderContext(RenderGraph& renderGraph, RenderGraphPass* currentPass, RefPtr<RHI::CommandBuffer> commandBuffer);

		void Flush(RefPtr<RHI::Fence> fence);

		void BeginRendering(const RenderingInfo2& renderingInfo);
		void EndRendering();

		const RenderingInfo2 CreateRenderingInfo(const uint32_t width, const uint32_t height, const ShaderParameterRenderTargetBindings& rtBindings);

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

		void BindPipeline(RefPtr<RHI::RenderPipeline> pipeline);
		void BindPipeline(RefPtr<RHI::ComputePipeline> pipeline);

		void BindIndexBuffer(RGBufferRef indexBuffer);
		void BindVertexBuffers(const StackVector<RGBufferRef, RHI::MAX_VERTEX_BUFFER_COUNT>& vertexBuffers, const uint32_t firstBinding);

		void CopyBufferRegion(RGBufferRef src, const size_t srcOffset, RGBufferRef dst, const size_t dstOffset, const size_t size);
		void CopyTexture(RGTextureRef src, RGTextureRef dst, const uint32_t width, const uint32_t height, const uint32_t depth);

		template<typename T> T* MapBuffer(RGBufferUAVRef buffer);
		template<typename T> T* MapBuffer(RGUniformBufferRef buffer);

		void UnmapBuffer(RGBufferUAVRef buffer);
		void UnmapBuffer(RGUniformBufferRef buffer);

		template<typename ShaderType> void SetParameters(RefPtr<RHI::Shader> shader, const typename ShaderType::Parameters* parameters);
		template<typename ParameterStruct> void CollectParameters(const ParameterStruct* parameters, BatchedShaderParameters& batchedShaderParameters);

		RefPtr<RHI::CommandBuffer> GetRHICommandBuffer();

	private:
		struct PerStageShaderParameters
		{
			RHI::ShaderStage shaderStage;
			RefPtr<RHI::UniformBuffer> uniformBuffer;
			uint8_t* mappedPtr;
		};

		void BindDescriptorTable();
		void AllocatePerStageShaderParameterBuffers();

		void SetBufferSRVParameter(RGBufferSRVRef bufferSRV, const ShaderParameterMetadata& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap);
		void SetBufferUAVParameter(RGBufferUAVRef bufferUAV, const ShaderParameterMetadata& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap);
		void SetTextureSRVParameter(RGTextureSRVRef textureSRV, const ShaderParameterMetadata& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap);
		void SetTextureUAVParameter(RGTextureUAVRef textureUAV, const ShaderParameterMetadata& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap);
		void SetUniformBufferParameter(RGUniformBufferRef uniformBuffer, const ShaderParameterMetadata& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap);
		void SetShaderParameter(const void* data, const ShaderParameterMetadata& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap);

		void CollectBufferSRVParameter(RGBufferSRVRef bufferSRV, const ShaderParameterMetadata& parameterMetadata, BatchedShaderParameters& batchedShaderParameters);
		void CollectBufferUAVParameter(RGBufferUAVRef bufferUAV, const ShaderParameterMetadata& parameterMetadata, BatchedShaderParameters& batchedShaderParameters);
		void CollectTextureSRVParameter(RGTextureSRVRef textureSRV, const ShaderParameterMetadata& parameterMetadata, BatchedShaderParameters& batchedShaderParameters);
		void CollectTextureUAVParameter(RGTextureUAVRef textureUAV, const ShaderParameterMetadata& parameterMetadata, BatchedShaderParameters& batchedShaderParameters);
		void CollectUniformBufferParameter(RGUniformBufferRef uniformBuffer, const ShaderParameterMetadata& parameterMetadata, BatchedShaderParameters& batchedShaderParameters);

		void* MapInternal(RGBufferUAVRef buffer);
		void* MapInternal(RGUniformBufferRef buffer);

		RawPtr<RHI::RenderPipeline> m_currentRenderPipeline;
		RawPtr<RHI::ComputePipeline> m_currentComputePipeline;
		RefPtr<RHI::CommandBuffer> m_commandBuffer;
		RefPtr<RHI::DescriptorTable> m_descriptorTable;

		// #TODO_Ivar: Move to an inline allocator
		PagedVector<PerStageShaderParameters> m_perStageShaderParameters;

		RenderGraph& m_renderGraph;
		RenderGraphPass* m_currentPass;
	};

	template<typename T>
	T* RenderContext::MapBuffer(RGBufferUAVRef buffer)
	{
		return reinterpret_cast<T*>(MapInternal(buffer));
	}

	template<typename T>
	T* RenderContext::MapBuffer(RGUniformBufferRef buffer)
	{
		return reinterpret_cast<T*>(MapInternal(buffer));
	}

	template<typename ShaderType>
	void RenderContext::SetParameters(RefPtr<RHI::Shader> shader, const typename ShaderType::Parameters* parameters)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(m_currentRenderPipeline || m_currentComputePipeline);

		using ShaderParametersType = typename ShaderType::Parameters;

		constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<ShaderType>();
		const auto& parameterRegistrationInfo = g_shaderRegistry.GetShaderRegistrationInfo(typeIndex);
		const auto& shaderParameterMap = shader->GetParameterMap();

		// We need to use const_cast here because the resource parameters need to be non-const pointers.
		uint8_t* parametersDataPtr = reinterpret_cast<uint8_t*>(const_cast<ShaderParametersType*>(parameters));

		for (const auto& parameter : parameterRegistrationInfo.parameterMetadata)
		{
			uint8_t* parameterDataPtr = &parametersDataPtr[parameter.structOffset];

			switch (parameter.parameterType)
			{
				case ShaderParameterType::BufferSRV: SetBufferSRVParameter(*reinterpret_cast<RGBufferSRVRef*>(parameterDataPtr), parameter, shaderParameterMap); break;
				case ShaderParameterType::BufferUAV: SetBufferUAVParameter(*reinterpret_cast<RGBufferUAVRef*>(parameterDataPtr), parameter, shaderParameterMap); break;
				case ShaderParameterType::TextureSRV: SetTextureSRVParameter(*reinterpret_cast<RGTextureSRVRef*>(parameterDataPtr), parameter, shaderParameterMap); break;
				case ShaderParameterType::TextureUAV: SetTextureUAVParameter(*reinterpret_cast<RGTextureUAVRef*>(parameterDataPtr), parameter, shaderParameterMap); break;
				case ShaderParameterType::UniformBuffer: SetUniformBufferParameter(*reinterpret_cast<RGUniformBufferRef*>(parameterDataPtr), parameter, shaderParameterMap); break;
				case ShaderParameterType::Parameter: SetShaderParameter(parameterDataPtr, parameter, shaderParameterMap); break;
			}
		}
	}

	template<typename ParameterStruct>
	void RenderContext::CollectParameters(const ParameterStruct* parameters, BatchedShaderParameters& batchedShaderParameters)
	{
		Vector<ShaderParameterMetadata> parameterStructMetadata;
		ParameterStruct::zzInternal_ProcessMembers(parameterStructMetadata);

		// We need to use const_cast here because the resource parameters need to be non-const pointers.
		uint8_t* parametersStructBytePtr = reinterpret_cast<uint8_t*>(const_cast<ParameterStruct*>(parameters));

		for (const auto& parameter : parameterStructMetadata)
		{
			uint8_t* parameterDataPtr = &parametersStructBytePtr[parameter.structOffset];

			switch (parameter.parameterType)
			{
				case ShaderParameterType::BufferSRV: CollectBufferSRVParameter(*reinterpret_cast<RGBufferSRVRef*>(parameterDataPtr), parameter, batchedShaderParameters); break;
				case ShaderParameterType::BufferUAV: CollectBufferUAVParameter(*reinterpret_cast<RGBufferUAVRef*>(parameterDataPtr), parameter, batchedShaderParameters); break;
				case ShaderParameterType::TextureSRV: CollectTextureSRVParameter(*reinterpret_cast<RGTextureSRVRef*>(parameterDataPtr), parameter, batchedShaderParameters); break;
				case ShaderParameterType::TextureUAV: CollectTextureUAVParameter(*reinterpret_cast<RGTextureUAVRef*>(parameterDataPtr), parameter, batchedShaderParameters); break;
				case ShaderParameterType::UniformBuffer: CollectUniformBufferParameter(*reinterpret_cast<RGUniformBufferRef*>(parameterDataPtr), parameter, batchedShaderParameters); break;
			}
		}
	}
}
