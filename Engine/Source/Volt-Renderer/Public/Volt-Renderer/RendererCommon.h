#pragma once

#include <glm/glm.hpp>

namespace Volt
{
	///// Rendering Structures /////
	struct ViewUniformBuffer
	{
		// Camera
		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 inverseView;
		glm::mat4 inverseProjection;
		glm::mat4 viewProjection;
		glm::mat4 inverseViewProjection;
		glm::mat4 prevViewProjection;
		glm::mat4 nonJitteredViewProjection;
		glm::vec4 cameraPosition;
		glm::vec4 cullingFrustum;
		glm::vec2 depthUnpackConsts;
		float nearPlane;
		float farPlane;
	
		glm::vec2 currentFrameJitter;
		glm::vec2 prevFrameJitter;

		// Render Target
		glm::uvec2 renderSize;
		glm::vec2 invRenderSize;

		// Light Culling
		uint32_t tileCountX;
		uint32_t lightCount;

		uint32_t frameIndex;
	};

	struct DirectionalLightShadowUniformBuffer
	{
		inline static constexpr uint32_t CASCADE_COUNT = 4;

		glm::mat4 viewProjections[CASCADE_COUNT];
		float cascadeDistances[CASCADE_COUNT];
	};
}
