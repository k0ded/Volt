#pragma once

#include <Volt-Renderer/MeshPassProcessor.h>

class OutlineMeshPassProcessor : public Volt::MeshPassProcessor
{
public:
	void AddRenderPrimitive(const Volt::RenderPrimitiveData* renderPrimitive) override;
	void RemoveRenderPrimitive(const Volt::RenderPrimitiveData* renderPrimitive) override;
};
