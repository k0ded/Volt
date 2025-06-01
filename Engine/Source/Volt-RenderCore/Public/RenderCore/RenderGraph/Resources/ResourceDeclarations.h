#pragma once

#include "RenderCore/RenderGraph/Resources/RenderGraphBuffer.h"
#include "RenderCore/RenderGraph/Resources/RenderGraphTexture.h"
#include "RenderCore/RenderGraph/Resources/RenderGraphUniformBuffer.h"

namespace Volt
{
	using RGBufferUAVRef = RGBufferUAV*;
	using RGBufferSRVRef = RGBufferSRV*;

	using RGTextureUAVRef = RGTextureUAV*;
	using RGTextureSRVRef = RGTextureSRV*;

	using RGUniformBufferRef = RGUniformBuffer*;
	
	using RGResourceSRVRef = RGResourceSRV*;
	using RGResourceUAVRef = RGResourceUAV*;
}
