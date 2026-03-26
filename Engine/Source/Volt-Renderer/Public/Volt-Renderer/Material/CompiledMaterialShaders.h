#pragma once

#include "Volt-Renderer/Config.h"

#include <CoreUtilities/TypeTraits/TypeIndex.h>
#include <CoreUtilities/Containers/Vector.h>
#include <string>

namespace Volt
{
	class CompiledMaterialShaders
	{
	public:
		struct CompiledMaterialShader
		{
			CompiledMaterialShader()
				: shaderType(TypeTraits::TypeIndex::FromType<void>())
			{}

			TypeTraits::TypeIndex shaderType;
			String compiledShader;
			String entryPoint;
		};

		VTR_API void Add(TypeTraits::TypeIndex shaderType, String&& compiledShader, const String& entryPoint);
		VTR_API const CompiledMaterialShader& Get(TypeTraits::TypeIndex shaderType) const;

		VT_INLINE bool Empty() const { return m_compiledShaders.empty(); }

	private:
		Vector<CompiledMaterialShader> m_compiledShaders;
	};
}
