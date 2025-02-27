#pragma once

#include "Volt-Renderer/Config.h"

#include <RHIModule/Images/Image.h>

#include <EntitySystem/EntityID.h>

#include <glm/glm.hpp>

namespace Volt
{
	class RenderScene;

	enum class SceneLightType : uint32_t
	{
		Directional = 0,
		Point,
		Spot,
		Sky
	};

	struct SceneLightDescription
	{
		SceneLightType lightType;

		// Point
		float radius;

		// Spot
		float range;
		float innerAngle;
		float outerAngle;

		// Directional
		float sunRadius;

		// Directional / Spot light
		glm::vec3 direction;

		// Skylight
		RefPtr<RHI::Image> diffuseIBL;
		RefPtr<RHI::Image> specularIBL;
		float lod;
		bool show;

		// Common
		float intensity;
		glm::vec3 color;
		float falloff;
		bool castShadows;
	};

	class VTR_API SceneLightData
	{
	public:
		SceneLightData(const EntityID& entityId, RenderScene* renderScene);
		~SceneLightData();

		void InitializeFromDescription(const SceneLightDescription& description);
		void Invalidate();

	private:
		void DestroyPrimitive();

		SceneLightDescription m_description;

		EntityID m_relatedEntity;
		RenderScene* m_renderScene;

		UUID64 m_renderSceneId = 0;
	};
}
