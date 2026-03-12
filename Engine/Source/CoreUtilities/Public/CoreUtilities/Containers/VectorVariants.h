#pragma once

#include "CoreUtilities/Containers/Vector.h"

template<typename T>
using GlobalMemoryStackVector = Vector<T, GlobalMemoryStackAllocator>;

template<typename T, size_t NumValues>
using InlineVector = Vector<T, InlineAllocator<NumValues>>;
