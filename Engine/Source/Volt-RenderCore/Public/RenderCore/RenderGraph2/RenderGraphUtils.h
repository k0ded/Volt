#pragma once

#include "RenderCore/Config.h"
#include "RenderCore/RenderGraph2/Resources/ResourceDeclarations.h"
#include "RenderCore/RenderGraph2/RenderGraph2.h"
#include "RenderCore/RenderGraph2/RenderContext2.h"
#include "RenderCore/Shader/PipelineStateCache.h"

namespace Volt
{
	class RenderGraph2;

	extern VTRC_API void AddCopyBufferPass(RenderGraph2& renderGraph, RGBufferRef src, const size_t srcOffset, RGBufferRef dst, const size_t dstOffset, const size_t size, const std::string& passName = "Copy Buffer");
	extern VTRC_API void AddMappedBufferUpload(RenderGraph2& renderGraph, RGBufferRef dst, const void* data, const size_t dataSize);
	extern VTRC_API void AddClearUAVPass(RenderGraph2& renderGraph, RGBufferUAVRef bufferUAV, const uint32_t clearValue);
	extern VTRC_API void AddClearUAVPass(RenderGraph2& renderGraph, RGBufferUAVRef bufferUAV, const float clearValue);

	namespace ComputeShaderUtils
	{
		template<typename ShaderType>
		void AddPass(RenderGraph2& renderGraph, const std::string& passName, RefPtr<RHI::Shader2> shader, const typename ShaderType::Parameters* passParameters, RenderGraphPassFlags flags, const glm::uvec3& dispatchSize)
		{
			renderGraph.AddPass(passName,
				RenderGraphPassFlags::Compute | flags,
				passParameters,
				[passParameters, shader, dispatchSize](RenderContext2& context)
			{
				auto pipeline = PipelineStateCache::GetComputePipeline(shader);

				context.BindPipeline(pipeline);
				context.SetParameters<ShaderType>(shader, passParameters);
				context.Dispatch(dispatchSize.x, dispatchSize.y, dispatchSize.z);
			});
		}

		template<typename ShaderType>
		void AddPass(RenderGraph2& renderGraph, const std::string& passName, RefPtr<RHI::Shader2> shader, const typename ShaderType::Parameters* passParameters, const glm::uvec3& dispatchSize)
		{
			AddPass<ShaderType>(renderGraph, passName, shader, passParameters, RenderGraphPassFlags::None, dispatchSize);
		}
	}
}
