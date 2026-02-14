#include "rcpch.h"

#include "RenderCore/RenderGraph/ShaderParameterStruct.h"

namespace Volt
{
	ShaderParameterMetadataDescription::ShaderParameterMetadataDescription(Vector<ShaderParameterMetadata>&& shaderParameterMetadata)
		: m_metadata(std::move(shaderParameterMetadata))
	{
	}
	
	ShaderParameterRenderTargetDecl::ShaderParameterRenderTargetDecl(RGTextureRef inTexture)
		: texture(inTexture)
	{
		subResourceRange.baseMipLevel = 0;
		subResourceRange.baseArrayLayer = 0;
		subResourceRange.mipCount = RHI::ImageViewDesc::MipCountMax;
		subResourceRange.layerCount = RHI::ImageViewDesc::LayerCountMax;
	}

	ShaderParameterRenderTargetDecl::ShaderParameterRenderTargetDecl(RGTextureRef inTexture, RGTextureSubResourceRange inSubResourceRange)
		: texture(inTexture),
		subResourceRange(inSubResourceRange)
	{

	}

	ShaderParameterRenderTargetDecl& ShaderParameterRenderTargetDecl::operator=(RGTextureRef inTexture)
	{
		texture = inTexture;
		subResourceRange.baseMipLevel = 0;
		subResourceRange.baseArrayLayer = 0;
		subResourceRange.mipCount = RHI::ImageViewDesc::MipCountMax;
		subResourceRange.layerCount = RHI::ImageViewDesc::LayerCountMax;

		return *this;
	}
}
