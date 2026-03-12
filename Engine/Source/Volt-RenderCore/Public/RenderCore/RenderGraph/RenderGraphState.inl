#pragma once

#include <RHIModule/RHIHelpers.h>

namespace Volt
{
	template<typename Func>
	void EnumerateTextureSubResources(const RGTextureSubResourceState& subResourceStates, Func&& func)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(subResourceStates.size()); ++i)
		{
			if (subResourceStates[i])
			{
				func(*subResourceStates[i], i);
			}
		}
	}

	template<typename Func>
	void EnumerateTextureSubResources(RGTextureSubResourceState& subResourceStates, Func&& func)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(subResourceStates.size()); ++i)
		{
			if (subResourceStates[i])
			{
				func(*subResourceStates[i], i);
			}
		}
	}

	template<typename Func>
	void RGTextureState::EnumerateSubResources(Func&& func)
	{
		EnumerateTextureSubResources(subResourceStates, std::move(func));
	}

	template<typename Func>
	void RGTextureState::EnumerateSubResources(Func&& func) const
	{
		EnumerateTextureSubResources(subResourceStates, std::move(func));
	}

	template<typename Func>
	void RGTextureState::EnumerateSubResourceRange(const RGTextureSubResourceRange& subResourceRange, Func&& func)
	{
		const RGTextureDesc& textureDesc = texture->GetDesc();

		const uint32_t numMips = subResourceRange.mipCount == RHI::ImageViewDesc::MipCountMax ? textureDesc.mips - subResourceRange.baseMipLevel : subResourceRange.mipCount;
		const uint32_t numLayers = subResourceRange.layerCount == RHI::ImageViewDesc::LayerCountMax ? textureDesc.layers - subResourceRange.baseArrayLayer : subResourceRange.layerCount;

		for (uint32_t mip = subResourceRange.baseMipLevel; mip < subResourceRange.baseMipLevel + numMips; ++mip)
		{
			for (uint32_t layer = subResourceRange.baseArrayLayer; layer < subResourceRange.baseArrayLayer + numLayers; ++layer)
			{
				func(subResourceStates[RHI::GetSubResourceIndex(mip, layer, 0, textureDesc.mips, textureDesc.layers)]);
			}
		}
	}
}
