#pragma once

#include <CoreUtilities/Allocators/PagedAtomicLinearAllocator.h>

namespace Volt
{
	using RenderGraphDataAllocator = PagedAtomicLinearAllocator<65536>;
}
