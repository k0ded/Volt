#pragma once

#include "Volt-Renderer/RenderScene/SceneLightData.h"

#include <RenderCore/RenderGraph/ShaderParameterStruct.h>
#include <RHIModule/Descriptors/BindlessIndex.h>

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

	struct GPUTransform
	{
		GPUTransform()
			: rotation(glm::identity<glm::quat>()),
			position(0.f),
			scale(1.f)
		{}

		glm::quat rotation;
		glm::vec3 position;
		float padding0;
		glm::vec3 scale;
		float padding1;
	};

	struct GPUMesh
	{
		inline static constexpr uint32_t MAX_LOD_COUNT = 8;

		glm::vec3 center;
		float radius;

		uint32_t vertexStartOffset;
		uint32_t indexStartOffset;
		glm::uvec2 padding;

		// Ray Tracing
		uint32_t RT_vertexPositionsBuffer;
		uint32_t RT_vertexMaterialBuffer;
		uint32_t RT_vertexAnimationInfoBuffer;
		uint32_t RT_indexBuffer;
	};

	struct PrimitiveDrawData
	{
		GPUTransform transform;

		uint32_t meshId;
		uint32_t materialId;
		uint32_t meshletStartOffset;
		uint32_t entityId;

		uint32_t isAnimated;
		uint32_t boneOffset;
		PrimitiveFlags flags;
		float padding;
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
		RHI::BindlessIndex textures[16];
		RHI::BindlessIndex samplers[16];

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
		SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<GPUMesh>, GPUMeshes)
		SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<LightDrawData>, SceneLights)
		SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<float4x4>, AnimatedBones)
	END_SHADER_PARAMETER_STRUCT()
}
