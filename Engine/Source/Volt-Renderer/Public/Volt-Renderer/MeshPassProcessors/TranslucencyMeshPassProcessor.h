#pragma once

#include "Volt-Renderer/MeshPassProcessor.h"

namespace Volt
{
	class TranslucencyMeshPassProcessor : public MeshPassProcessor
	{
	public:
		void AddRenderPrimitive(const RenderPrimitiveData* renderPrimitive) override;
		void RemoveRenderPrimitive(const RenderPrimitiveData* renderPrimitive) override;

	private:
		bool ShouldIncludePrimitive(const RenderPrimitiveData* renderPrimitive) const;
	};
}
