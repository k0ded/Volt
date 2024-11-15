#pragma once

#include <glm/glm.hpp>

#include <RHIModule/Images/Image.h>

namespace Volt
{
	struct SceneEnvironment
	{
		RefPtr<RHI::Image> diffuse;
		RefPtr<RHI::Image> specular;

		float lod = 0.f;
		float intensity = 1.f;
	};
}
