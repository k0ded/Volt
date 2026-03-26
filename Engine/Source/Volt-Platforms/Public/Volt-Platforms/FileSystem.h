#pragma once

#include <CoreUtilities/Core.h>

#include <cstdint>

enum class CopyOptions : uint8_t
{
	None = 0,
	Recursive = 1 << 0,
	CopySymlinks = 1 << 1,
	SkipExisting = 1 << 2,
	DirectoriesOnly = 1 << 3
};
VT_SETUP_ENUM_CLASS_OPERATORS(CopyOptions);
