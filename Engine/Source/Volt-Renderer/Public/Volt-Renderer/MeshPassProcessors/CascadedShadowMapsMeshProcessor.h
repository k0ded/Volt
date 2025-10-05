#pragma once

#include "Volt-Renderer/MeshPassProcessor.h"

namespace Volt
{
	class CascadedShadowMapMeshProcessor : public MeshPassProcessor
	{
	public:
		void AddRenderPrimitive(const RenderPrimitiveData& renderPrimitive) override;
		void RemoveRenderPrimitive(const RenderPrimitiveData& renderPrimitive) override;
	};
}
