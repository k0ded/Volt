#pragma once

#include <CoreUtilities/Math/Hash.h>

#include <xhash>

namespace Volt
{
	struct MeshletCone
	{
		int8_t x;
		int8_t y;
		int8_t z;
		int8_t cutoff;
	};

	struct MeshletVertexTriangleCount
	{
		uint32_t vertexCount : 16;
		uint32_t triangleCount : 16;
	};

	struct Meshlet
	{
		MeshletVertexTriangleCount vertexTriCount;
		uint32_t meshId;
		uint32_t dataOffset;
		MeshletCone cone;

		glm::vec3 boundingSphereCenter;
		float boundingSphereRadius;
	};

	struct MeshletNew
	{
		glm::vec3 center;
		float radius;
		int8_t coneAxis[3];
		int8_t coneCutoff;

		uint32_t dataOffset;
		uint8_t vertexCount;
		uint8_t triangleCount;
	};

	struct Edge
	{
		uint32_t v0;
		uint32_t v1;

		inline bool operator==(const Edge& rhs) const
		{
			return v0 == rhs.v0 && v1 == rhs.v1;
		}
	};

	struct VertexMaterialData
	{
		uint32_t normal;
		float tangent = 0.f;
		float tangentW = 1.f; 
		uint32_t texCoords = 0;
	};

	struct VertexAnimationInfo
	{
		uint16_t influenceCount;
		uint16_t boneOffset;
	};

	struct VertexAnimationData
	{
		glm::uvec4 influences = 0u;
		glm::vec4 weights = 0.f;
	};
}

namespace std
{
	template<typename T> struct hash;

	template<>
	struct hash<Volt::Edge>
	{
		std::size_t operator()(const Volt::Edge& edge) const
		{
			return Math::HashCombine(std::hash<uint32_t>()(edge.v0), std::hash<uint32_t>()(edge.v1));
		}
	};
}
