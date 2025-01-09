#pragma once

#include "Volt-Renderer/Config.h"

#include <CoreUtilities/Core.h>

namespace Volt
{
	class Mesh;
	class VTR_API ShapeLibrary
	{
	public:
		static Ref<Mesh> GetCube();
		static Ref<Mesh> GetSphere();

	private:
		ShapeLibrary() = delete;
	};
}
