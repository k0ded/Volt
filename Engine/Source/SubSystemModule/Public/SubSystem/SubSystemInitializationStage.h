#pragma once

#include <cstdint>

enum class SubSystemInitializationStage : uint8_t
{
	PreEngine = 0,
	Engine,
	PostEngine
};

enum class SubSystemInclusionLevel : uint8_t
{
	Minimal = 0,
	Default = 1
};
