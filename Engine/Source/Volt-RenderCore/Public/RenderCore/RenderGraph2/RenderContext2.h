#pragma once

#include "RenderCore/Config.h"
#include "RenderCore/RenderGraph/ShaderRegistry.h"

#include <RHIModule/Pipelines/RenderPipeline.h>
#include <RHIModule/Pipelines/ComputePipeline.h>
#include <RHIModule/Buffers/CommandBuffer.h>

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	class RenderGraph2;
	class SharedRenderContext;
	class RenderGraphPass;

	struct RenderingInfo2
	{
		RHI::Rect2D scissor{};
		RHI::Viewport viewport{};
		RHI::RenderingInfo renderingInfo{};
	};

	class VTRC_API RenderContext2
	{
	public:
		RenderContext2(RenderGraph2& renderGraph, SharedRenderContext& sharedRenderContext, RenderGraphPass* currentPass, RefPtr<RHI::CommandBuffer> commandBuffer);

		void BeginRendering(const RenderingInfo2& renderingInfo);
		void EndRendering();

		const RenderingInfo2 CreateRenderingInfo(const uint32_t width, const uint32_t height, const ShaderParameterRenderTargetBindings& rtBindings);

		void Dispatch(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ);

		void Draw(const uint32_t vertexCount, const uint32_t instanceCount, const uint32_t firstVertex, const uint32_t firstInstance);

		void BindPipeline(RefPtr<RHI::RenderPipeline> pipeline);
		void BindPipeline(RefPtr<RHI::ComputePipeline> pipeline);

		template<typename ShaderType> 
		void SetParameters(RefPtr<RHI::Shader2> shader,  const typename ShaderType::Parameters* parameters)
		{
			VT_PROFILE_FUNCTION();
			VT_ENSURE(m_currentRenderPipeline || m_currentComputePipeline);

			using ShaderParametersType = typename ShaderType::Parameters;

			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<ShaderType>();
			const auto& parameterRegistrationInfo = g_shaderRegistry.GetShaderRegistrationInfo2(typeIndex);
			const auto& shaderParameterMap = shader->GetParameterMap();

			// We need to use const_cast here because the resource parameters need to be non-const pointers.
			uint8_t* parametersDataPtr = reinterpret_cast<uint8_t*>(const_cast<ShaderParametersType*>(parameters));

			for (const auto& parameter : parameterRegistrationInfo.parameterMetadata)
			{
				uint8_t* parameterDataPtr = &parametersDataPtr[parameter.structOffset];

				switch (parameter.parameterType)
				{
					case ShaderParameterType2::BufferSRV: SetBufferSRVParameter(*reinterpret_cast<RGBufferSRVRef*>(parameterDataPtr), parameter, shaderParameterMap); break;
					case ShaderParameterType2::BufferUAV: SetBufferUAVParameter(*reinterpret_cast<RGBufferUAVRef*>(parameterDataPtr), parameter, shaderParameterMap); break;
					case ShaderParameterType2::TextureSRV: SetTextureSRVParameter(*reinterpret_cast<RGTextureSRVRef*>(parameterDataPtr), parameter, shaderParameterMap); break;
					case ShaderParameterType2::TextureUAV: SetTextureUAVParameter(*reinterpret_cast<RGTextureUAVRef*>(parameterDataPtr), parameter, shaderParameterMap); break;
					case ShaderParameterType2::Parameter: SetShaderParameter(parameterDataPtr, parameter, shaderParameterMap); break;
				}
			}
		}

	private:
		struct PerStageShaderParameters
		{
			RHI::ShaderStage shaderStage;
			RefPtr<RHI::UniformBuffer> uniformBuffer;
			uint8_t* mappedPtr;
		};

		void BindDescriptorTable();
		void AllocatePerStageShaderParameterBuffers();

		void SetBufferSRVParameter(RGBufferSRVRef bufferSRV, const ShaderParameterMetadata2& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap);
		void SetBufferUAVParameter(RGBufferUAVRef bufferUAV, const ShaderParameterMetadata2& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap);
		void SetTextureSRVParameter(RGTextureSRVRef textureSRV, const ShaderParameterMetadata2& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap);
		void SetTextureUAVParameter(RGTextureUAVRef textureUAV, const ShaderParameterMetadata2& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap);

		void SetShaderParameter(const void* data, const ShaderParameterMetadata2& parameterMetadata, const RHI::ShaderParameterMap& shaderParameterMap);

		RawPtr<RHI::RenderPipeline> m_currentRenderPipeline;
		RawPtr<RHI::ComputePipeline> m_currentComputePipeline;
		RefPtr<RHI::CommandBuffer> m_commandBuffer;
		RefPtr<RHI::DescriptorTable> m_descriptorTable;

		// #TODO_Ivar: Move to an inline allocator
		PagedVector<PerStageShaderParameters> m_perStageShaderParameters;

		RenderGraph2& m_renderGraph;
		SharedRenderContext& m_sharedRenderContext;
		RenderGraphPass* m_currentPass;
	};
}
