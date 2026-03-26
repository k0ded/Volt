#include "rcpch.h"

#include "RenderCore/RenderGraph/RenderGraph.h"
#include "RenderCore/RenderGraph/RenderContext.h"
#include "RenderCore/RenderGraph/RenderGraphUtils.h"
#include "RenderCore/RenderGraph/ShaderParameterStruct.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/GraphicsDevice.h>
#include <RHIModule/Graphics/DeviceQueue.h>

namespace Volt
{
	BEGIN_SHADER_PARAMETER_STRUCT(CopyBufferParameters)
		RG_BUFFER_ACCESS(CopySrc, RGResourceAccess::CopySrc)
		RG_BUFFER_ACCESS(CopyDst, RGResourceAccess::CopyDst)
	END_SHADER_PARAMETER_STRUCT()

	BEGIN_SHADER_PARAMETER_STRUCT(CopyTextureParameters)
		RG_TEXTURE_ACCESS(CopySrc, RGResourceAccess::CopySrc)
		RG_TEXTURE_ACCESS(CopyDst, RGResourceAccess::CopyDst)
	END_SHADER_PARAMETER_STRUCT()

	BEGIN_SHADER_PARAMETER_STRUCT(MappedBufferUploadParameters)
		RG_BUFFER_ACCESS(Buffer, RGResourceAccess::Upload)
	END_SHADER_PARAMETER_STRUCT()

	BEGIN_SHADER_PARAMETER_STRUCT(MappedUniformBufferUploadParameters)
		RG_UNIFORM_BUFFER_ACCESS(Buffer, RGResourceAccess::Upload)
	END_SHADER_PARAMETER_STRUCT()

	BEGIN_SHADER_PARAMETER_STRUCT(StagedBufferUploadParameters)
		RG_BUFFER_ACCESS(StagingBuffer, RGResourceAccess::CopySrc)
		RG_BUFFER_ACCESS(DstBuffer, RGResourceAccess::CopyDst)
	END_SHADER_PARAMETER_STRUCT()

	BEGIN_SHADER_PARAMETER_STRUCT(ClearBufferUAVParameters)
		SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWBuffer)
	END_SHADER_PARAMETER_STRUCT()

	BEGIN_SHADER_PARAMETER_STRUCT(ClearTextureUAVParameters)
		SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float4>, RWTexture)
	END_SHADER_PARAMETER_STRUCT()

	void ValidateTextureCopy(RGTextureRef src, RGTextureRef dst)
	{
		const RGTextureDesc& srcDesc = src->GetDesc();
		const RGTextureDesc& dstDesc = dst->GetDesc();

		VT_ENSURE_MSG(srcDesc.format == dstDesc.format, "Source and Destination textures must have the same format!");
		VT_ENSURE_MSG(srcDesc.width == dstDesc.width && srcDesc.height == dstDesc.height && srcDesc.depth == dstDesc.depth, "Source and Destination textures must have the same dimensions!");
		VT_ENSURE_MSG(srcDesc.imageType == dstDesc.imageType, "Source and Destination textures must be of the same type!");
	}

	void ValidateBufferCopy(RGBufferRef src, size_t srcOffset, RGBufferRef dst, size_t dstOffset, size_t size)
	{
		const RGBufferDesc srcDesc = src->GetDesc();
		const RGBufferDesc dstDesc = dst->GetDesc();
	
		const size_t srcByteSize = srcDesc.numElements * srcDesc.elementSize;
		const size_t dstByteSize = dstDesc.numElements * dstDesc.elementSize;

		VT_ENSURE_MSG(srcOffset < srcByteSize, "Source offset must be less than Source size!");
		VT_ENSURE_MSG(dstOffset < dstByteSize, "Destination offset must be less than Destination size!");

		VT_ENSURE_MSG((srcByteSize - srcOffset) >= size, "Source size - Source offset must be greater than, or equal to the copied size!");
		VT_ENSURE_MSG((dstByteSize - dstOffset) >= size, "Destination size - Destination offset must be greater than, or equal to the copied size!");
	}

	void AddCopyBufferPass(RenderGraph& renderGraph, RGBufferRef src, size_t srcOffset, RGBufferRef dst, size_t dstOffset, size_t size)
	{
		ValidateBufferCopy(src, srcOffset, dst, dstOffset, size);

		CopyBufferParameters* parameters = renderGraph.AllocParameters<CopyBufferParameters>();
		parameters->CopySrc = src;
		parameters->CopyDst = dst;

		const String passName = FormatString("Copy Buffer (Src: {}, Dst: {})", src->GetDesc().debugName, dst->GetDesc().debugName);

		renderGraph.AddPass(passName,
			RenderGraphPassFlags::Copy,
			parameters,
			[parameters, srcOffset, dstOffset, size](RenderContext& context) 
		{
			context.CopyBufferRegion(parameters->CopySrc, srcOffset, parameters->CopyDst, dstOffset, size);
		});
	}

	void AddCopyTexturePass(RenderGraph& renderGraph, RGTextureRef src, RGTextureRef dst)
	{
		ValidateTextureCopy(src, dst);

		CopyTextureParameters* parameters = renderGraph.AllocParameters<CopyTextureParameters>();
		parameters->CopySrc = src;
		parameters->CopyDst = dst;

		const String passName = FormatString("Copy Texture (Src: {}, Dst: {})", src->GetDesc().debugName, dst->GetDesc().debugName);

		renderGraph.AddPass(passName,
			RenderGraphPassFlags::Copy,
			parameters,
			[parameters](RenderContext& context)
		{
			const RGTextureDesc& srcDesc = parameters->CopySrc->GetDesc();
			context.CopyTexture(parameters->CopySrc, parameters->CopyDst, srcDesc.width, srcDesc.height, srcDesc.depth);
		});
	}

	void AddMappedBufferUploadCopyData(RenderGraph& renderGraph, RGBufferRef dstBuffer, const void* data, const size_t dataSize, RenderGraphPassFlags flags)
	{
		void* tempData = renderGraph.AllocData(dataSize);
		memcpy_s(tempData, dataSize, data, dataSize);

		const String passName = FormatString("Mapped Upload (Target: {})", dstBuffer->GetDesc().debugName);

		MappedBufferUploadParameters* stagingParameters = renderGraph.AllocParameters<MappedBufferUploadParameters>();
		stagingParameters->Buffer = dstBuffer;

		renderGraph.AddPass(passName,
			RenderGraphPassFlags::Compute | RenderGraphPassFlags::NeverCull | flags,
			stagingParameters,
			[stagingParameters, tempData, dataSize](RenderContext& context)
		{
			uint8_t* mappedPtr = context.MapBuffer<uint8_t>(stagingParameters->Buffer);
			memcpy_s(mappedPtr, dataSize, tempData, dataSize);
			context.UnmapBuffer(stagingParameters->Buffer);
		});
	}

	void AddStagedBufferUploadCopyData(RenderGraph& renderGraph, RGBufferRef dstBuffer, const void* data, uint64_t dataSize, RenderGraphPassFlags flags /*= RenderGraphPassFlags::None*/)
	{
		void* tempData = renderGraph.AllocData(dataSize);
		memcpy_s(tempData, dataSize, data, dataSize);

		const String passName = FormatString("Staged Upload (Target: {})", dstBuffer->GetDesc().debugName);

		RGBufferRef stagingBuffer = renderGraph.CreateBuffer(RGBufferDesc::CreateStagingDesc(dataSize, "StagingBuffer"));

		StagedBufferUploadParameters* stagingParameters = renderGraph.AllocParameters<StagedBufferUploadParameters>();
		stagingParameters->DstBuffer = dstBuffer;
		stagingParameters->StagingBuffer = stagingBuffer;

		renderGraph.AddPass(passName,
			RenderGraphPassFlags::Copy | RenderGraphPassFlags::NeverCull | flags,
			stagingParameters,
			[stagingParameters, tempData, dataSize](RenderContext& context) 
		{
			void* mappedPtr = context.MapBuffer<uint8_t>(stagingParameters->StagingBuffer);
			memcpy_s(mappedPtr, dataSize, tempData, dataSize);
			context.UnmapBuffer(stagingParameters->StagingBuffer);

			context.CopyBufferRegion(stagingParameters->StagingBuffer, 0, stagingParameters->DstBuffer, 0, dataSize);
		});
	}

	void AddMappedBufferUpload(RenderGraph& renderGraph, RGBufferRef dstBuffer, const void* data, const size_t dataSize, RenderGraphPassFlags flags /*= RenderGraphPassFlags::None*/)
	{
		const String passName = FormatString("Mapped Upload (Target: {})", dstBuffer->GetDesc().debugName);

		MappedBufferUploadParameters* stagingParameters = renderGraph.AllocParameters<MappedBufferUploadParameters>();
		stagingParameters->Buffer = dstBuffer;

		renderGraph.AddPass(passName,
			RenderGraphPassFlags::Compute | RenderGraphPassFlags::NeverCull | flags,
			stagingParameters,
			[stagingParameters, data, dataSize](RenderContext& context)
		{
			uint8_t* mappedPtr = context.MapBuffer<uint8_t>(stagingParameters->Buffer);
			memcpy_s(mappedPtr, dataSize, data, dataSize);
			context.UnmapBuffer(stagingParameters->Buffer);
		});
	}

	void AddMappedBufferUploadCopyData(RenderGraph& renderGraph, RGUniformBufferRef dstUniformBuffer, const void* data, const size_t dataSize, RenderGraphPassFlags flags)
	{
		void* tempData = renderGraph.AllocData(dataSize);
		memcpy_s(tempData, dataSize, data, dataSize);

		const String passName = FormatString("Mapped Upload (Target: {})", dstUniformBuffer->GetDesc().debugName);

		MappedUniformBufferUploadParameters* stagingParameters = renderGraph.AllocParameters<MappedUniformBufferUploadParameters>();
		stagingParameters->Buffer = dstUniformBuffer;

		renderGraph.AddPass(passName,
			RenderGraphPassFlags::Compute | RenderGraphPassFlags::NeverCull | flags,
			stagingParameters,
			[stagingParameters, tempData, dataSize](RenderContext& context)
		{
			uint8_t* mappedPtr = context.MapBuffer<uint8_t>(stagingParameters->Buffer);
			memcpy_s(mappedPtr, dataSize, tempData, dataSize);
			context.UnmapBuffer(stagingParameters->Buffer);
		});
	}

	void AddClearUAVPass(RenderGraph& renderGraph, RGBufferUAVRef bufferUAV, const uint32_t clearValue)
	{
		RGBufferRef targetBuffer = reinterpret_cast<RGBufferRef>(bufferUAV->GetResource());
		const String passName = FormatString("Clear Buffer UAV (Target: {})", targetBuffer->GetDesc().debugName);

		ClearBufferUAVParameters* parameters = renderGraph.AllocParameters<ClearBufferUAVParameters>();
		parameters->RWBuffer = bufferUAV;

		renderGraph.AddPass(passName,
			RenderGraphPassFlags::Clear,
			parameters,
			[parameters, clearValue](RenderContext& context) 
		{
			context.ClearUAV(parameters->RWBuffer, clearValue);
		});
	}

	void AddClearUAVPass(RenderGraph& renderGraph, RGBufferUAVRef bufferUAV, const float clearValue)
	{
		RGBufferRef targetBuffer = reinterpret_cast<RGBufferRef>(bufferUAV->GetResource());
		const String passName = FormatString("Clear Buffer UAV (Target: {})", targetBuffer->GetDesc().debugName);

		ClearBufferUAVParameters* parameters = renderGraph.AllocParameters<ClearBufferUAVParameters>();
		parameters->RWBuffer = bufferUAV;

		renderGraph.AddPass(passName,
			RenderGraphPassFlags::Clear,
			parameters,
			[parameters, clearValue](RenderContext& context)
		{
			context.ClearUAV(parameters->RWBuffer, clearValue);
		});
	}

	void AddClearUAVPass(RenderGraph& renderGraph, RGTextureUAVRef textureUAV, const glm::uvec4& clearValue)
	{
		RGTextureRef targetTexture = reinterpret_cast<RGTextureRef>(textureUAV->GetResource());
		const String passName = FormatString("Clear Texture UAV (Target: {})", targetTexture->GetDesc().debugName);

		ClearTextureUAVParameters* parameters = renderGraph.AllocParameters<ClearTextureUAVParameters>();
		parameters->RWTexture = textureUAV;

		renderGraph.AddPass(passName,
			RenderGraphPassFlags::Clear,
			parameters,
			[parameters, clearValue](RenderContext& context)
		{
			context.ClearUAV(parameters->RWTexture, clearValue);
		});
	}

	void AddClearUAVPass(RenderGraph& renderGraph, RGTextureUAVRef textureUAV, const glm::vec4& clearValue)
	{
		RGTextureRef targetTexture = reinterpret_cast<RGTextureRef>(textureUAV->GetResource());
		const String passName = FormatString("Clear Texture UAV (Target: {})", targetTexture->GetDesc().debugName);

		ClearTextureUAVParameters* parameters = renderGraph.AllocParameters<ClearTextureUAVParameters>();
		parameters->RWTexture = textureUAV;

		renderGraph.AddPass(passName,
			RenderGraphPassFlags::Clear,
			parameters,
			[parameters, clearValue](RenderContext& context)
		{
			context.ClearUAV(parameters->RWTexture, clearValue);
		});
	}
}
