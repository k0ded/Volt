#pragma once

template<typename AllocatorType>
struct ContainerAllocatorTraits
{
	inline static constexpr bool RequiresAllocatorCopy = false;
};
