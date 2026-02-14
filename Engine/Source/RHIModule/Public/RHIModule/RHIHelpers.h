#pragma once

#include <cstdint>

#include <CoreUtilities/CompilerTraits.h>

namespace Volt::RHI
{
	VT_INLINE uint32_t GetSubResourceIndex(uint32_t mipIndex, uint32_t layerIndex, uint32_t planeIndex, uint32_t mipCount, uint32_t layerCount)
	{
		return mipIndex + (layerIndex * mipCount) + (planeIndex * mipCount * layerCount);
	}

	VT_INLINE void GetSubResourceFromIndex(uint32_t subResourceIndex, uint32_t mipCount, uint32_t layerCount, uint32_t& outMipIndex, uint32_t& outLayerIndex, uint32_t& outPlaneIndex)
	{
		outPlaneIndex = subResourceIndex / (mipCount * layerCount);
		subResourceIndex -= (outPlaneIndex * mipCount * layerCount);
		outLayerIndex = subResourceIndex / mipCount;
		outMipIndex = subResourceIndex % mipCount;
	}
}
