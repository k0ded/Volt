#pragma once

#include "CoreUtilities/Containers/Vector.h"
#include "CoreUtilities/Allocators/FrameStackAllocator.h"

template<typename T>
using FrameStackVector = Vector<T, FrameStackAllocator::Mark>;

template<typename T, size_t NumValues>
using InlineVector = Vector<T, InlineAllocator<NumValues>>;
