#pragma once

#include "ComponentVisualizers/ComponentVisualizer.h"
#include "ComponentVisualizers/ComponentVisualizerRegistry.h"

#include "Sandbox/Utility/EditorResources.h"
#include "Sandbox/Utility/SelectionManager.h"

#include "Sandbox/EditorAssetManager.h"


#include <Volt-Assets/MaterialAsset.h>

#include <Volt-CoreComponents/LightComponents.h>
#include <Volt-CoreComponents/RenderingComponents.h>

#include <Volt-Physics/ColliderComponents.h>

class PointLightComponentVisualizer : public ComponentVisualizer<Volt::PointLightComponent>
{
public:
	void DrawVisualization(EditorDrawInterface& editorDrawInterface, Volt::PointLightComponent& component, Volt::Entity entity)
	{
		editorDrawInterface.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::LightGizmo), entity.GetTransformTQS());
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(PointLightComponentVisualizer);

class SpotLightComponentVisualizer : public ComponentVisualizer<Volt::SpotLightComponent>
{
public:
	void DrawVisualization(EditorDrawInterface& editorDrawInterface, Volt::SpotLightComponent& component, Volt::Entity entity)
	{
		editorDrawInterface.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::LightGizmo), entity.GetTransformTQS());
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(SpotLightComponentVisualizer);

class SphereLightComponentVisualizer : public ComponentVisualizer<Volt::SphereLightComponent>
{
public:
	void DrawVisualization(EditorDrawInterface& editorDrawInterface, Volt::SphereLightComponent& component, Volt::Entity entity)
	{
		editorDrawInterface.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::LightGizmo), entity.GetTransformTQS());
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(SphereLightComponentVisualizer);

class RectangleLightComponentVisualizer : public ComponentVisualizer<Volt::RectangleLightComponent>
{
public:
	void DrawVisualization(EditorDrawInterface& editorDrawInterface, Volt::RectangleLightComponent& component, Volt::Entity entity)
	{
		editorDrawInterface.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::LightGizmo), entity.GetTransformTQS());
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(RectangleLightComponentVisualizer);

class DirectionalLightComponentVisualizer : public ComponentVisualizer<Volt::DirectionalLightComponent>
{
public:
	void DrawVisualization(EditorDrawInterface& editorDrawInterface, Volt::DirectionalLightComponent& component, Volt::Entity entity)
	{
		editorDrawInterface.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::LightGizmo), entity.GetTransformTQS());
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(DirectionalLightComponentVisualizer);

class SkyLightComponentVisualizer : public ComponentVisualizer<Volt::SkylightComponent>
{
public:
	void DrawVisualization(EditorDrawInterface& editorDrawInterface, Volt::SkylightComponent& component, Volt::Entity entity)
	{
		editorDrawInterface.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::LightGizmo), entity.GetTransformTQS());
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(SkyLightComponentVisualizer);

class CameraComponentVisualizer : public ComponentVisualizer<Volt::CameraComponent>
{
public:
	void DrawVisualization(EditorDrawInterface& editorDrawInterface, Volt::CameraComponent& component, Volt::Entity entity)
	{
		editorDrawInterface.DrawMesh(
			EditorResources::GetEditorMesh(EditorMesh::Camera),
			EditorResources::GetEditorMesh(EditorMesh::Camera)->GetMaterialTable().GetMaterial(0),
			entity.GetTransformTQS());
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(CameraComponentVisualizer);

struct BoxColliderComponentHitProxyContext
{
	glm::vec3 dir;
};
class BoxColliderComponentVisualizer : public ComponentVisualizer<Volt::BoxColliderComponent>
{
public:
	void DrawVisualization(EditorDrawInterface& editorDrawInterface, Volt::BoxColliderComponent& component, Volt::Entity entity)
	{
		//todo: this should not be hard-coded!
		const Volt::AssetHandle colliderVisMaterialHandle = 8190457749457779134;
		AssetReference<Volt::MaterialAsset> colliderVisMaterial = g_editorAssetManager->GetAssetImmediatelyAndCache<Volt::MaterialAsset>(colliderVisMaterialHandle);

		if (!colliderVisMaterial)
		{
			editorDrawInterface.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::Warning), entity.GetTransformTQS());
			return;
		}

		constexpr float CUBE_MESH_HALF_SIDE = 50.f;
		const glm::vec3 scaledOffset = component.offset * entity.GetScale();
		const glm::vec3 scaledHalfSize = component.halfSize * entity.GetScale();
		const glm::vec3 boxUnscaledExtentsMeters = component.halfSize / CUBE_MESH_HALF_SIDE;

		TQS boxTransform = entity.GetTransformTQS();
		boxTransform.translation += scaledOffset;
		boxTransform.scale.x *= boxUnscaledExtentsMeters.x;
		boxTransform.scale.y *= boxUnscaledExtentsMeters.y;
		boxTransform.scale.z *= boxUnscaledExtentsMeters.z;
		editorDrawInterface.DrawMesh(
			EditorResources::GetEditorMesh(EditorMesh::Cube),
			colliderVisMaterial->GetRenderMaterial(),
			boxTransform);

		if (!SelectionManager::IsSelected(entity.GetID()))
		{
			return;
		}

		TQS extendSphereTransform = boxTransform;
		const Vector<glm::vec3, InlineAllocator<6>> sideDirs =
		{
			glm::vec3(1,0,0), glm::vec3(-1,0,0),
			glm::vec3(0,1,0), glm::vec3(0,-1,0),
			glm::vec3(0,0,1), glm::vec3(0,0,-1)
		};

		BoxColliderComponentHitProxyContext context;
		for (const glm::vec3& dir : sideDirs)
		{
			context.dir = dir;

			float minSideScale = 1.f;
			if (dir.x != 0)
			{
				minSideScale = glm::min(dir.y, dir.z);
			}
			else if (dir.y != 0)
			{
				minSideScale = glm::min(dir.x, dir.z);
			}
			else if (dir.z != 0)
			{
				minSideScale = glm::min(dir.x, dir.y);
			}

			constexpr float SPHERE_SCALE_MULTIPLIER = 0.3f;
			constexpr float SPHERE_MIN_SCALE = 0.1f;
			const float sphereScale = glm::max(SPHERE_MIN_SCALE, (minSideScale * SPHERE_SCALE_MULTIPLIER));
			extendSphereTransform.scale = glm::vec3(sphereScale);

			constexpr float SPHERE_MESH_RADIUS = 50.f;
			constexpr float SPHERE_OFFSET_MULTIPLIER = 1.25f;
			extendSphereTransform.translation = boxTransform.translation + dir * (scaledHalfSize + sphereScale * SPHERE_MESH_RADIUS * SPHERE_OFFSET_MULTIPLIER);
			editorDrawInterface.DrawMesh<BoxColliderComponentVisualizer>(
				EditorResources::GetEditorMesh(EditorMesh::Sphere),
				EditorResources::GetEditorMesh(EditorMesh::Sphere)->GetMaterialTable().GetMaterial(0),
				extendSphereTransform,
				context);
		}
	}

	void HandleVisProxyInteraction(Volt::BoxColliderComponent& component, Volt::Entity entity, const BoxColliderComponentHitProxyContext& context)
	{
		component.halfSize += context.dir * 10.f;
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(BoxColliderComponentVisualizer);
