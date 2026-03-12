#include "vtassetspch.h"

#include "Volt-Assets/SourceAssetImporters/TangentGenerator.h"

#include <MikkTSpace/mikktspace.h>

namespace Volt::TangentGenerator
{
	static GenerationData& GetGenerationData(const SMikkTSpaceContext* ctx)
	{
		return *static_cast<GenerationData*>(ctx->m_pUserData);
	}

	static int32_t GetNumVerticesPerFace(const SMikkTSpaceContext* ctx, int)
	{
		return 3;
	}

	static int32_t GetNumFaces(const SMikkTSpaceContext* ctx)
	{
		GenerationData& generationData = GetGenerationData(ctx);
		return static_cast<int32_t>(generationData.indexCount) / GetNumVerticesPerFace(ctx, 0);
	}

	static void GetPosition(const SMikkTSpaceContext* ctx, float* outPos, int32_t faceIndex, int32_t vertexIndex)
	{
		GenerationData& generationData = GetGenerationData(ctx);
		uint32_t index = generationData.indices[faceIndex * GetNumVerticesPerFace(ctx, 0) + vertexIndex];
		memcpy_s(outPos, sizeof(glm::vec3), &generationData.vertexPositions[index], sizeof(glm::vec3));
	}

	static void GetNormal(const SMikkTSpaceContext* ctx, float* outNormal, int32_t faceIndex, int32_t vertexIndex)
	{
		GenerationData& generationData = GetGenerationData(ctx);
		uint32_t index = generationData.indices[faceIndex * GetNumVerticesPerFace(ctx, 0) + vertexIndex];
		memcpy_s(outNormal, sizeof(glm::vec3), &generationData.vertexNormals[index], sizeof(glm::vec3));
	}

	static void GetTexCoord(const SMikkTSpaceContext* ctx, float* outTexCoord, int32_t faceIndex, int32_t vertexIndex)
	{
		GenerationData& generationData = GetGenerationData(ctx);
		uint32_t index = generationData.indices[faceIndex * GetNumVerticesPerFace(ctx, 0) + vertexIndex];
		memcpy_s(outTexCoord, sizeof(glm::vec2), &generationData.vertexUvs[index], sizeof(glm::vec2));
	}

	static void SetTangent(const SMikkTSpaceContext* ctx, const float* tangent, float sign, int32_t faceIndex, int32_t vertexIndex)
	{
		GenerationData& generationData = GetGenerationData(ctx);
		uint32_t index = generationData.indices[faceIndex * GetNumVerticesPerFace(ctx, 0) + vertexIndex];
		generationData.outTangents[index] = glm::vec4(tangent[0], tangent[1], tangent[2], sign);
	}

	void GenerateTangents(GenerationData& generationData)
	{
		VT_ENSURE(generationData.indexCount > 0 &&
			generationData.vertexPositions != nullptr &&
			generationData.vertexNormals != nullptr &&
			generationData.vertexUvs != nullptr &&
			generationData.indices != nullptr);

		SMikkTSpaceInterface mikkTInterface{};
		mikkTInterface.m_getNumFaces = GetNumFaces;
		mikkTInterface.m_getNumVerticesOfFace = GetNumVerticesPerFace;
		mikkTInterface.m_getPosition = GetPosition;
		mikkTInterface.m_getNormal = GetNormal;
		mikkTInterface.m_getTexCoord = GetTexCoord;
		mikkTInterface.m_setTSpaceBasic = SetTangent;

		SMikkTSpaceContext ctx = {};
		ctx.m_pInterface = &mikkTInterface;
		ctx.m_pUserData = &generationData;

		genTangSpaceDefault(&ctx);
	}
}
