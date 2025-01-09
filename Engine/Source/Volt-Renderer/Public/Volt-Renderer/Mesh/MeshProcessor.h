#pragma once

#include "Volt-Renderer/MaterialTable.h"
#include "Volt-Renderer/Mesh/SubMesh.h"
#include "Volt-Renderer/Mesh/MeshCommon.h"
#include "Volt-Renderer/Vertex.h"

#include <span>

namespace Volt
{
	struct ProcessedMeshResult
	{
		Vector<MeshletNew> meshlets;
		Vector<uint32_t> meshletIndices; // Index and vertex remapping
	};

	struct MeshletGenerationResult
	{
		Vector<MeshletNew> meshlets;
		Vector<uint32_t> meshletIndices; // Index and vertex remapping
		Vector<uint32_t> meshletVertices;

		Vector<uint32_t> nonOffsetedIndices;
	};

	class MeshProcessor
	{
	public:
		static ProcessedMeshResult ProcessMesh(const Vector<Vertex>& vertices, const Vector<uint32_t>& indices, const MaterialTable& materialTable, const Vector<SubMesh>& subMeshes);

	private:
		static MeshletGenerationResult GenerateMeshlets(const Vertex* vertices, const uint32_t* indices, uint32_t indexCount, uint32_t vertexCount);
		static MeshletGenerationResult GenerateMeshlets2(std::span<const Vertex> vertices, std::span<const uint32_t> indices);

		MeshProcessor() = delete;
	};
}
