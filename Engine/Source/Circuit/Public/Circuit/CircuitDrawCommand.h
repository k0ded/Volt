#pragma once
#include "Circuit/Config.h"
#include "Circuit/CircuitColor.h"

#include <RHIModule/Descriptors/ResourceHandle.h>

#include <glm/vec2.hpp>

namespace Circuit
{
	enum class CircuitPrimitiveType : uint32_t
	{
		Circle = 0,
		Rect,
		Line,
		CircleSegment,
		TextCharacter,
		Image
	};

	struct CircuitDrawCommand
	{
		CircuitPrimitiveType type;
		int32_t primitiveGroup;

		// Common
		float rotation;
		float scale;

		glm::vec4 bounds;
		glm::vec4 clipRect;

		glm::vec2 position;

		float glowDistance;
		float glowStrength;

		glm::vec2 shadowOffset;
		float shadowStrength;
		float padding0;

		CircuitColor color;
		uint32_t textureIndex;

		// Rounding
		float rounding;

		// Circle
		float radius;

		// Rect
		glm::vec2 halfSize;

		// Circle Segment
		float radiusInner;
		float angle;

		// Line
		glm::vec2 lineA;
		glm::vec2 lineB;

		// Image
		glm::uvec2 dimensions;
		glm::vec2 padding1;

		// Text
		glm::vec4 minMaxUV;
		glm::vec4 minMaxPx;

		static CircuitDrawCommand Initialize()
		{
			CircuitDrawCommand result;
			result.primitiveGroup = -1;
			result.glowDistance = 0.f;
			result.glowStrength = 0.f;
			result.shadowOffset = 0.f;
			result.shadowStrength = 0.f;
			result.rounding = 0.f;
			result.radiusInner = 0.f;
			result.bounds = 0;
			result.clipRect = { -FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX };

			return result;
		}
	};
};
