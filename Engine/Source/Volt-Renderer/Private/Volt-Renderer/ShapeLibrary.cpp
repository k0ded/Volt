#include "vrpch.h"

#include "Volt-Renderer/ShapeLibrary.h"
#include "Volt-Renderer/Renderer.h"
#include "Volt-Renderer/Mesh/Mesh.h"

namespace Volt
{
	struct MeshData
	{
		Ref<Mesh> cubeMesh;
		Ref<Mesh> sphereMesh;
	};

	static MeshData s_meshData;


	static Ref<Mesh> CreateCube()
	{
		VertexContainer vertices;
		{
			// Front face
			vertices.Add(glm::vec3{ -50.f,  50.f, -50.f }, VertexMaterialData::Pack({ 0.f, 0.f, -1.f }, { 1.f, 0.f, 0.f, 0.f }, { 0.f, 0.f }), {});
			vertices.Add(glm::vec3{  50.f,  50.f, -50.f }, VertexMaterialData::Pack({ 0.f, 0.f, -1.f }, { 1.f, 0.f, 0.f, 0.f }, { 1.f, 0.f }), {});
			vertices.Add(glm::vec3{  50.f, -50.f, -50.f }, VertexMaterialData::Pack({ 0.f, 0.f, -1.f }, { 1.f, 0.f, 0.f, 0.f }, { 1.f, 1.f }), {});
			vertices.Add(glm::vec3{ -50.f, -50.f, -50.f }, VertexMaterialData::Pack({ 0.f, 0.f, -1.f }, { 1.f, 0.f, 0.f, 0.f }, { 0.f, 1.f }), {});

			// Right face
			vertices.Add(glm::vec3{  50.f,  50.f, -50.f }, VertexMaterialData::Pack({ 1.f, 0.f, 0.f }, { 0.f, 0.f, 1.f, 0.f }, { 0.f, 0.f }), {});
			vertices.Add(glm::vec3{  50.f,  50.f,  50.f }, VertexMaterialData::Pack({ 1.f, 0.f, 0.f }, { 0.f, 0.f, 1.f, 0.f }, { 1.f, 0.f }), {});
			vertices.Add(glm::vec3{  50.f, -50.f,  50.f }, VertexMaterialData::Pack({ 1.f, 0.f, 0.f }, { 0.f, 0.f, 1.f, 0.f }, { 1.f, 1.f }), {});
			vertices.Add(glm::vec3{  50.f, -50.f, -50.f }, VertexMaterialData::Pack({ 1.f, 0.f, 0.f }, { 0.f, 0.f, 1.f, 0.f }, { 0.f, 1.f }), {});

			// Back face
			vertices.Add(glm::vec3{  50.f,  50.f,  50.f }, VertexMaterialData::Pack({ 0.f, 0.f, 1.f }, { -1.f, 0.f, 0.f, 0.f }, { 0.f, 0.f }), {});
			vertices.Add(glm::vec3{ -50.f,  50.f,  50.f }, VertexMaterialData::Pack({ 0.f, 0.f, 1.f }, { -1.f, 0.f, 0.f, 0.f }, { 1.f, 0.f }), {});
			vertices.Add(glm::vec3{ -50.f, -50.f,  50.f }, VertexMaterialData::Pack({ 0.f, 0.f, 1.f }, { -1.f, 0.f, 0.f, 0.f }, { 1.f, 1.f }), {});
			vertices.Add(glm::vec3{  50.f, -50.f,  50.f }, VertexMaterialData::Pack({ 0.f, 0.f, 1.f }, { -1.f, 0.f, 0.f, 0.f }, { 0.f, 1.f }), {});

			// Left face
			vertices.Add(glm::vec3{ -50.f,  50.f,  50.f }, VertexMaterialData::Pack({ -1.f, 0.f, 0.f }, { 0.f, 0.f, -1.f, 0.f }, { 0.f, 0.f }), {});
			vertices.Add(glm::vec3{ -50.f,  50.f, -50.f }, VertexMaterialData::Pack({ -1.f, 0.f, 0.f }, { 0.f, 0.f, -1.f, 0.f }, { 1.f, 0.f }), {});
			vertices.Add(glm::vec3{ -50.f, -50.f, -50.f }, VertexMaterialData::Pack({ -1.f, 0.f, 0.f }, { 0.f, 0.f, -1.f, 0.f }, { 1.f, 1.f }), {});
			vertices.Add(glm::vec3{ -50.f, -50.f,  50.f }, VertexMaterialData::Pack({ -1.f, 0.f, 0.f }, { 0.f, 0.f, -1.f, 0.f }, { 0.f, 1.f }), {});

			// Top face
			vertices.Add(glm::vec3{ -50.f,  50.f,  50.f }, VertexMaterialData::Pack({ 0.f, 1.f, 0.f }, { 1.f, 0.f, 0.f, 0.f }, { 0.f, 0.f }), {});
			vertices.Add(glm::vec3{  50.f,  50.f,  50.f }, VertexMaterialData::Pack({ 0.f, 1.f, 0.f }, { 1.f, 0.f, 0.f, 0.f }, { 1.f, 0.f }), {});
			vertices.Add(glm::vec3{  50.f,  50.f, -50.f }, VertexMaterialData::Pack({ 0.f, 1.f, 0.f }, { 1.f, 0.f, 0.f, 0.f }, { 1.f, 1.f }), {});
			vertices.Add(glm::vec3{ -50.f,  50.f, -50.f }, VertexMaterialData::Pack({ 0.f, 1.f, 0.f }, { 1.f, 0.f, 0.f, 0.f }, { 0.f, 1.f }), {});

			// Bottom face
			vertices.Add(glm::vec3{ -50.f, -50.f, -50.f }, VertexMaterialData::Pack({ 0.f, -1.f, 0.f }, { 1.f, 0.f, 0.f, 0.f }, { 0.f, 0.f }), {});
			vertices.Add(glm::vec3{  50.f, -50.f, -50.f }, VertexMaterialData::Pack({ 0.f, -1.f, 0.f }, { 1.f, 0.f, 0.f, 0.f }, { 1.f, 0.f }), {});
			vertices.Add(glm::vec3{  50.f, -50.f,  50.f }, VertexMaterialData::Pack({ 0.f, -1.f, 0.f }, { 1.f, 0.f, 0.f, 0.f }, { 1.f, 1.f }), {});
			vertices.Add(glm::vec3{ -50.f, -50.f,  50.f }, VertexMaterialData::Pack({ 0.f, -1.f, 0.f }, { 1.f, 0.f, 0.f, 0.f }, { 0.f, 1.f }), {});
		}

		Vector<uint32_t> indices =
		{
			// Front face
			0, 1, 3,
			3, 1, 2,

			// Right face
			4, 5, 7,
			7, 5, 6,

			// Back face
			8, 9, 11,
			11, 9, 10,

			// Left face
			12, 13, 15,
			15, 13, 14,

			// Top face
			16, 17, 19,
			19, 17, 18,

			// Bottom face
			20, 21, 23,
			23, 21, 22
		};
		
		SubMesh subMesh;
		subMesh.vertexCount = static_cast<uint32_t>(vertices.Size());
		subMesh.indexCount = static_cast<uint32_t>(indices.size());
		subMesh.indexStartOffset = 0;
		subMesh.vertexStartOffset = 0;
		subMesh.materialIndex = 0;

		MeshInitializer meshInitializer;
		meshInitializer.AddVertices(vertices);
		meshInitializer.AddIndices(indices);
		meshInitializer.AddMaterial(Renderer::GetDefaultResources().defaultMaterial, 0);
		meshInitializer.AddSubMesh(subMesh);

		Ref<Mesh> mesh = CreateRef<Mesh>();
		mesh->Initialize(meshInitializer);
		return mesh;
	}

	static Ref<Mesh> CreateSphere()
	{
		struct Triangle
		{
			uint32_t v1, v2, v3;

			Triangle(uint32_t v1, uint32_t v2, uint32_t v3)
				: v1(v1), v2(v2), v3(v3)
			{
			}
		};
		
		auto addVertex = [](VertexContainer& vertices, const glm::vec3& position, const glm::vec2& texCoords) -> uint32_t
		{
			vertices.Add(position, VertexMaterialData::Pack({ 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f, 0.f }, texCoords), {});
			return static_cast<uint32_t>(vertices.Size()) - 1;
		};

		auto subdivide = [&addVertex](auto subdivide, VertexContainer& vertices, Vector<Triangle>& triangles, const uint32_t& v1, const uint32_t& v2, const uint32_t& v3, int32_t depth)
		{
			if (depth == 0)
			{
				triangles.emplace_back(v1, v2, v3);
				return;
			}

			const uint32_t middle1 = addVertex(vertices, glm::normalize(vertices.positions[v1] + vertices.positions[v2]), glm::vec2(0.0f, 0.0f));
			const uint32_t middle2 = addVertex(vertices, glm::normalize(vertices.positions[v2] + vertices.positions[v3]), glm::vec2(0.5f, 0.0f));
			const uint32_t middle3 = addVertex(vertices, glm::normalize(vertices.positions[v3] + vertices.positions[v1]), glm::vec2(1.0f, 0.0f));

			subdivide(subdivide, vertices, triangles, v1, middle1, middle3, depth - 1);
			subdivide(subdivide, vertices, triangles, middle1, v2, middle2, depth - 1);
			subdivide(subdivide, vertices, triangles, middle3, middle2, v3, depth - 1);
			subdivide(subdivide, vertices, triangles, middle1, middle2, middle3, depth - 1);
		};

		const float t = (1.f + std::sqrt(5.f)) / 2.f;

		const glm::vec3 icosahedronVertices[12] =
		{
			glm::normalize(glm::vec3(-1, t, 0)),
			glm::normalize(glm::vec3(1, t, 0)),
			glm::normalize(glm::vec3(-1, -t, 0)),
			glm::normalize(glm::vec3(1, -t, 0)),

			glm::normalize(glm::vec3(0, -1, t)),
			glm::normalize(glm::vec3(0, 1, t)),
			glm::normalize(glm::vec3(0, -1, -t)),
			glm::normalize(glm::vec3(0, 1, -t)),

			glm::normalize(glm::vec3(t, 0, -1)),
			glm::normalize(glm::vec3(t, 0, 1)),
			glm::normalize(glm::vec3(-t, 0, -1)),
			glm::normalize(glm::vec3(-t, 0, 1))
		};

		Triangle icosahedronIndices[20] = 
		{
			{0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
			{1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},

			{3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
			{4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
		};

		constexpr int32_t SUBDIVISIONS = 3;

		Vector<Triangle> triangles;
		VertexContainer vertices;

		// Add vertices of the icosahedron
		for (const glm::vec3& vertex : icosahedronVertices)
		{
			vertices.positions.push_back(vertex);
			vertices.materialData.emplace_back();
			vertices.animationData.emplace_back();
		}

		// Subdivide each face of the icosahedron
		for (const auto& triangle : icosahedronIndices)
		{
			subdivide(subdivide, vertices, triangles, triangle.v1, triangle.v2, triangle.v3, SUBDIVISIONS);
		}

		// Calculate normals, tangents, and update UVs
		for (size_t i = 0; i < vertices.Size(); ++i)
		{
			const glm::vec3 position = vertices.positions.at(i);
			const glm::vec3 normal = glm::normalize(position);
			const glm::vec4 tangent = glm::vec4(glm::normalize(glm::cross(normal, glm::vec3(0.0f, 1.0f, 0.0f))), 0.f);

			// Calculate UVs using spherical coordinates
			float theta = std::atan2(position.z, position.x) + glm::pi<float>();
			float phi = std::acos(position.y);

			const glm::vec2 uv = glm::vec2(theta / (2.0f * glm::pi<float>()), phi / glm::pi<float>());

			vertices.materialData[i] = VertexMaterialData::Pack(normal, tangent, uv);
		}

		Vector<uint32_t> indices;
		for (const auto& tri : triangles)
		{
			indices.emplace_back(tri.v1);
			indices.emplace_back(tri.v2);
			indices.emplace_back(tri.v3);
		}

		SubMesh subMesh;
		subMesh.vertexCount = static_cast<uint32_t>(vertices.Size());
		subMesh.indexCount = static_cast<uint32_t>(indices.size());
		subMesh.indexStartOffset = 0;
		subMesh.vertexStartOffset = 0;
		subMesh.materialIndex = 0;

		MeshInitializer meshInitializer;
		meshInitializer.AddVertices(vertices);
		meshInitializer.AddIndices(indices);
		meshInitializer.AddMaterial(Renderer::GetDefaultResources().defaultMaterial, 0);
		meshInitializer.AddSubMesh(subMesh);

		Ref<Mesh> mesh = CreateRef<Mesh>();
		mesh->Initialize(meshInitializer);
		return mesh;
	}

	void ShapeLibrary::Shutdown()
	{
		s_meshData = {};
	}

	Ref<Mesh> ShapeLibrary::GetCube()
	{
		if (!s_meshData.cubeMesh)
		{
			s_meshData.cubeMesh = CreateCube();
		}

		return s_meshData.cubeMesh;
	}

	Ref<Mesh> ShapeLibrary::GetSphere()
	{
		if (!s_meshData.sphereMesh)
		{
			s_meshData.sphereMesh = CreateSphere();
		}

		return s_meshData.sphereMesh;
	}
}
