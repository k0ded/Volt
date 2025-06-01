#include "rcpch.h"

#include "RenderCore/RenderGraph2/RenderGraph2.h"
#include "RenderCore/RenderGraph2/RenderContext2.h"
#include "RenderCore/RenderGraph2/RenderGraphUtils.h"
#include "RenderCore/RenderGraph2/ShaderParameterStruct2.h"

namespace Volt
{
	BEGIN_SHADER_PARAMETER_STRUCT(CopyBufferParameters)
		RG_BUFFER_ACCESS(CopySrc, RGResourceAccess::CopySrc)
		RG_BUFFER_ACCESS(CopyDst, RGResourceAccess::CopyDst)
	END_SHADER_PARAMETER_STRUCT()

	void AddCopyBufferPass(RenderGraph2& renderGraph, RGBufferRef src, const size_t srcOffset, RGBufferRef dst, const size_t dstOffset, const size_t size, const std::string& passName)
	{
		CopyBufferParameters* parameters = renderGraph.AllocParameters<CopyBufferParameters>();
		parameters->CopySrc = src;
		parameters->CopyDst = dst;

		renderGraph.AddPass(passName,
			RenderGraphPassFlags::None,
			parameters,
			[parameters, srcOffset, dstOffset, size](RenderContext2& context) 
		{
			context.CopyBufferRegion(parameters->CopySrc, srcOffset, parameters->CopyDst, dstOffset, size);
		});
	}

	BEGIN_SHADER_PARAMETER_STRUCT(MappedBufferUploadParameters)
		SHADER_PARAMETER_BUFFER_UAV(RGBufferUAV, RWBuffer)
	END_SHADER_PARAMETER_STRUCT()

	void AddMappedBufferUpload(RenderGraph2& renderGraph, RGBufferRef dst, const void* data, const size_t dataSize)
	{
		void* tempData = renderGraph.AllocData(dataSize);
		memcpy_s(tempData, dataSize, data, dataSize);

		MappedBufferUploadParameters* stagingParameters = renderGraph.AllocParameters<MappedBufferUploadParameters>();
		stagingParameters->RWBuffer = renderGraph.CreateUAV(dst);

		renderGraph.AddPass("Mapped Upload",
			RenderGraphPassFlags::Compute,
			stagingParameters,
			[stagingParameters, tempData, dataSize](RenderContext2& context)
		{
			uint8_t* mappedPtr = context.MapBuffer<uint8_t>(stagingParameters->RWBuffer);
			memcpy_s(mappedPtr, dataSize, tempData, dataSize);
			context.UnmapBuffer(stagingParameters->RWBuffer);
		});
	}

	BEGIN_SHADER_PARAMETER_STRUCT(ClearBufferUAVParameters)
		SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWBuffer)
	END_SHADER_PARAMETER_STRUCT()

	void AddClearUAVPass(RenderGraph2& renderGraph, RGBufferUAVRef bufferUAV, const uint32_t clearValue)
	{
		ClearBufferUAVParameters* parameters = renderGraph.AllocParameters<ClearBufferUAVParameters>();
		parameters->RWBuffer = bufferUAV;

		renderGraph.AddPass("Clear Buffer UAV",
			RenderGraphPassFlags::Compute,
			parameters,
			[parameters, clearValue](RenderContext2& context) 
		{
			context.ClearUAV(parameters->RWBuffer, clearValue);
		});
	}

	void AddClearUAVPass(RenderGraph2& renderGraph, RGBufferUAVRef bufferUAV, const float clearValue)
	{
		ClearBufferUAVParameters* parameters = renderGraph.AllocParameters<ClearBufferUAVParameters>();
		parameters->RWBuffer = bufferUAV;

		renderGraph.AddPass("Clear Buffer UAV",
			RenderGraphPassFlags::Compute,
			parameters,
			[parameters, clearValue](RenderContext2& context)
		{
			context.ClearUAV(parameters->RWBuffer, clearValue);
		});
	}
}
