#pragma once

#include "Volt-Renderer/MeshPassProcessor.h"

namespace Volt
{
	class BasePassMeshProcessor : public MeshPassProcessor
	{
	public:
		void AddRenderPrimitive(const RenderPrimitiveData* renderPrimitive) override;
		void RemoveRenderPrimitive(const RenderPrimitiveData* renderPrimitive) override;
		bool ShouldIncludePrimitive(const RenderPrimitiveData* renderPrimitive) const override;
	};
}
