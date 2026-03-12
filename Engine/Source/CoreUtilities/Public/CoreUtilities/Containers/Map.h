#pragma once

#include "ankerl/unordered_dense.h"

template<typename Key, typename Value>
using Map = ankerl::unordered_dense::map<Key, Value>;
