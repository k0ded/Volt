#pragma once

#include "CoreUtilities/Containers/Vector.h"
#include "CoreUtilities/Allocators/GlobalMemoryStack.h"

template<typename T>
using GlobalMemoryStackVector = Vector<T, GlobalMemoryStackAllocator>;

template<typename T, size_t NumValues>
using InlineVector = Vector<T, InlineAllocator<NumValues>>;
