#pragma once

#include <glm/glm.hpp>

namespace Volt::TangentGenerator
{
	struct GenerationData
	{
		const glm::vec3* vertexPositions = nullptr;
		const glm::vec3* vertexNormals = nullptr;
		const glm::vec2* vertexUvs = nullptr;
		const uint32_t* indices = nullptr;

		glm::vec4* outTangents = nullptr;
		uint32_t indexCount;
	};

	void GenerateTangents(GenerationData& generationData);
}
