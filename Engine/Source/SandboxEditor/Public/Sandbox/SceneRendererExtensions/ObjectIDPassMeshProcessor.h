#pragma once

#include <Volt-Renderer/MeshPassProcessor.h>

class ObjectIDPassMeshProcessor : public Volt::MeshPassProcessor
{
public:
	void AddRenderPrimitive(const Volt::RenderPrimitiveData& renderPrimitive) override;
	void RemoveRenderPrimitive(UUID64 renderPrimitveId) override;
};
