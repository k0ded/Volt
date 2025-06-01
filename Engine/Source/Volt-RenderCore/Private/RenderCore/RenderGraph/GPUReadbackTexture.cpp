#include "rcpch.h"
#include "RenderCore/RenderGraph/GPUReadbackTexture.h"

#include "RenderCore/RenderGraph/Resources/RenderGraphTexture.h"

#include <RHIModule/Images/Image.h>

namespace Volt
{
	GPUReadbackTexture::GPUReadbackTexture(const RGTextureDesc& desc)
	{
		RHI::ImageSpecification spec{};
		spec.width = desc.width;
		spec.height = desc.height;
		spec.depth = desc.depth;
		spec.layers = desc.layers;
		spec.mips = desc.mips;

		spec.format = desc.format;
		spec.usage = desc.usage;
		spec.debugName = desc.debugName;
		spec.isCubeMap = desc.isCubeMap;
		spec.initializeImage = false;
		spec.memoryUsage = RHI::MemoryUsage::GPUToCPU;

		m_image = RHI::Image::Create(spec, nullptr);
	}
}
