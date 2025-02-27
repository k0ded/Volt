#pragma once

#include <CoreUtilities/UUID.h>

#include <yaml-cpp/yaml.h>

namespace Volt
{
	using AssetHandle = UUID64;
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, const Volt::AssetHandle& handle)
{
	out << static_cast<uint64_t>(handle);
	return out;
}
