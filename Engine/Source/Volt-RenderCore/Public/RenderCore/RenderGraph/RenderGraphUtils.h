#pragma once

#include "RenderCore/Config.h"
#include "RenderCore/RenderGraph/Resources/ResourceDeclarations.h"
#include "RenderCore/RenderGraph/RenderGraph.h"
#include "RenderCore/RenderGraph/RenderContext.h"
#include "RenderCore/Shader/PipelineStateCache.h"

namespace Volt
{
	class RenderGraph;

	extern VTRC_API void AddCopyBufferPass(RenderGraph& renderGraph, RGBufferRef src, size_t srcOffset, RGBufferRef dst, size_t dstOffset, size_t size);
	extern VTRC_API void AddCopyTexturePass(RenderGraph& renderGraph, RGTextureRef src, RGTextureRef dst);

	/*
		Will copy the data into temporary storage in the Render Graph.
	*/
	extern VTRC_API void AddMappedBufferUploadCopyData(RenderGraph& renderGraph, RGBufferRef dstBuffer, const void* data, const size_t dataSize, RenderGraphPassFlags flags = RenderGraphPassFlags::None);
	extern VTRC_API void AddMappedBufferUploadCopyData(RenderGraph& renderGraph, RGUniformBufferRef dstUniformBuffer, const void* data, const size_t dataSize, RenderGraphPassFlags flags = RenderGraphPassFlags::None);

	/*
		Will not copy the data into temporary storage, so the data MUST at least have the same lifetime as the Render Graph.
	*/
	extern VTRC_API void AddMappedBufferUpload(RenderGraph& renderGraph, RGBufferRef dstBuffer, const void* data, const size_t dataSize, RenderGraphPassFlags flags = RenderGraphPassFlags::None);

	extern VTRC_API void AddClearUAVPass(RenderGraph& renderGraph, RGBufferUAVRef bufferUAV, const uint32_t clearValue);
	extern VTRC_API void AddClearUAVPass(RenderGraph& renderGraph, RGBufferUAVRef bufferUAV, const float clearValue);
	extern VTRC_API void AddClearUAVPass(RenderGraph& renderGraph, RGTextureUAVRef textureUAV, const glm::uvec4& clearValue);
	extern VTRC_API void AddClearUAVPass(RenderGraph& renderGraph, RGTextureUAVRef textureUAV, const glm::vec4& clearValue);

	namespace ComputeShaderUtils
	{
		template<typename ShaderType>
		void AddPass(RenderGraph& renderGraph, const std::string& passName, RefPtr<RHI::Shader> shader, const typename ShaderType::Parameters* passParameters, RenderGraphPassFlags flags, const glm::uvec3& dispatchSize)
		{
			renderGraph.AddPass(passName,
				RenderGraphPassFlags::Compute | flags,
				passParameters,
				[passParameters, shader, dispatchSize](RenderContext& context)
			{
				context.SetPipelineState(shader);
				context.SetParameters<ShaderType>(shader, passParameters);
				context.Dispatch(dispatchSize.x, dispatchSize.y, dispatchSize.z);
			});
		}

		template<typename ShaderType>
		void AddPass(RenderGraph& renderGraph, const std::string& passName, RefPtr<RHI::Shader> shader, const typename ShaderType::Parameters* passParameters, const glm::uvec3& dispatchSize)
		{
			AddPass<ShaderType>(renderGraph, passName, shader, passParameters, RenderGraphPassFlags::None, dispatchSize);
		}

		template<typename ShaderType>
		void AddPass(RenderGraph& renderGraph, const std::string& passName, RefPtr<RHI::Shader> shader, const typename ShaderType::Parameters* passParameters, RenderGraphPassFlags flags, RGBufferRef indirectArgsBuffer, uint64_t argsOffset)
		{
			renderGraph.AddPass(passName,
				RenderGraphPassFlags::Compute | flags,
				passParameters,
				[passParameters, shader, indirectArgsBuffer, argsOffset](RenderContext& context)
			{
				context.SetPipelineState(shader);
				context.SetParameters<ShaderType>(shader, passParameters);
				context.DispatchIndirect(indirectArgsBuffer, argsOffset);
			});
		}

		template<typename ShaderType>
		void AddPass(RenderGraph& renderGraph, const std::string& passName, RefPtr<RHI::Shader> shader, const typename ShaderType::Parameters* passParameters, RGBufferRef indirectArgsBuffer, uint64_t argsOffset)
		{
			AddPass<ShaderType>(renderGraph, passName, shader, passParameters, RenderGraphPassFlags::None, indirectArgsBuffer, argsOffset);
		}
	}
}
