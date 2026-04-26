#pragma once

#if BINDLESS_ENABLED
namespace BindlessResources
{
	template<typename T>
	T Get(uint index)
	{
		return ResourceDescriptorHeap[NonUniformResourceIndex(index)];
	}

	SamplerState GetSampler(uint index)
	{
		return SamplerDescriptorHeap[NonUniformResourceIndex(index)];
	}
}
#endif