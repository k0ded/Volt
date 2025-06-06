#pragma once

#include "Volt-Renderer/RenderScene/SceneLightData.h"

#include <RenderCore/RenderGraph/ShaderParameterStruct.h>

#include <RHIModule/Descriptors/ResourceHandle.h>

#include <glm/glm.hpp>
#include <cstdint>

namespace Volt
{
	enum class PrimitiveFlags : uint32_t
	{
		None = 0,
		Valid = BIT(0),
		Invalid = BIT(1)
	};

	VT_SETUP_ENUM_CLASS_OPERATORS(PrimitiveFlags);

	enum class LightFlags : uint32_t
	{
		None = 0,
		Invalid = BIT(0),
		CastShadows = BIT(1)
	};

	VT_SETUP_ENUM_CLASS_OPERATORS(LightFlags);

	struct GPUMesh
	{
		inline static constexpr uint32_t MAX_LOD_COUNT = 8;

		ResourceHandle vertexPositionsBuffer;
		ResourceHandle vertexMaterialBuffer;
		ResourceHandle vertexAnimationInfoBuffer;
		ResourceHandle vertexBoneInfluencesBuffer;
		ResourceHandle indexBuffer;

		ResourceHandle vertexBoneWeightsBuffer;
		ResourceHandle meshletDataBuffer;
		ResourceHandle meshletsBuffer;

		glm::vec3 center;
		float radius;

		uint32_t vertexStartOffset;
		uint32_t meshletCount;
		uint32_t meshletStartOffset;
		uint32_t meshletIndexStartOffset;
	};

	struct GPUMeshSDF
	{
		ResourceHandle sdfTexture;
		glm::vec3 size;

		glm::vec3 min;
		glm::vec3 max;
		ResourceHandle bricksBuffer;
		uint32_t brickCount;
	};

	struct PrimitiveDrawData
	{
		glm::quat rotation;
		glm::vec3 position;
		glm::vec3 scale;

		uint32_t meshId;
		uint32_t materialId;
		uint32_t meshletStartOffset;
		uint32_t entityId;

		uint32_t isAnimated;
		uint32_t boneOffset;
		PrimitiveFlags flags;
		glm::uvec2 padding;
	};

	struct SDFPrimitiveDrawData
	{
		glm::quat rotation;
		glm::vec3 position;
		glm::vec3 scale;

		uint32_t meshSDFId;
		uint32_t primtiveId;
	};

	struct GPUSDFBrick
	{
		glm::vec3 min;
		glm::vec3 max;

		glm::vec3 localCoord;
	};

	struct GPUMaterial
	{
		ResourceHandle textures[16];
		ResourceHandle samplers[16];

		uint32_t textureCount = 0;
		uint32_t materialFlags = 0;
		glm::uvec2 padding;
	};

	struct LightDrawData
	{
		SceneLightType lightType;
		glm::vec3 position;

		LightFlags flags;
		glm::vec3 direction;

		float intensity;
		glm::vec3 color;

		// Point: .x=radius, .y=falloff
		// Spot: .x=range, .w=falloff, .y=lightAngleScale .z=lightAngleOffset
		// Dir: .x=angularRadius
		// Sky: .x=LOD
		glm::vec4 lightSpecific;
	};

	BEGIN_SHADER_PARAMETER_STRUCT(GPUSceneParameters)
		SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<PrimitiveDrawData>, PrimitiveDrawDataBuffer)
		SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<PrimitiveDrawData>, PrevPrimitiveDrawDataBuffer)
	END_SHADER_PARAMETER_STRUCT()
}
