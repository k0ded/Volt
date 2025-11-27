#pragma once

#include "RenderCore/Config.h"
#include "RenderCore/RenderGraph/Resources/ResourceDeclarations.h"
#include "RenderCore/RenderGraph/RenderGraph.h"
#include "RenderCore/RenderGraph/RenderContext.h"
#include "RenderCore/Shader/PipelineStateCache.h"

namespace Volt
{
	class RenderGraph;

	extern VTRC_API void AddCopyBufferPass(RenderGraph& renderGraph, RGBufferRef src, const size_t srcOffset, RGBufferRef dst, const size_t dstOffset, const size_t size, const std::string& passName = "Copy Buffer");
	extern VTRC_API void AddMappedBufferUpload(RenderGraph& renderGraph, RGBufferUAVRef dstUAV, const void* data, const size_t dataSize, RenderGraphPassFlags flags = RenderGraphPassFlags::None);
	extern VTRC_API void AddMappedBufferUpload(RenderGraph& renderGraph, RGUniformBufferRef dstUAV, const void* data, const size_t dataSize, RenderGraphPassFlags flags = RenderGraphPassFlags::None);
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
				auto pipeline = PipelineStateCache::GetComputePipeline(shader);

				context.BindPipeline(pipeline);
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
				auto pipeline = PipelineStateCache::GetComputePipeline(shader);

				context.BindPipeline(pipeline);
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
