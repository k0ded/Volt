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
		AssetReference<Volt::MaterialAsset> colliderVisMaterial;
		if (!g_editorAssetManager->TryGetAssetImmediatelyAndCache<Volt::MaterialAsset>("Editor/Materials/M_ColliderVisualization.vtasset", colliderVisMaterial))
		{
			editorDrawInterface.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::Warning), entity.GetTransformTQS());
			return;
		}

		constexpr float CUBE_MESH_HALF_SIDE = 50.f;
		const glm::vec3 scaledOffset = component.offset * entity.GetScale();
		const glm::vec3 scaledHalfSize = component.halfSize * entity.GetScale();
		const glm::vec3 boxUnscaledExtentsMeters = component.halfSize / CUBE_MESH_HALF_SIDE;

		TQS boxTransform = entity.GetTransformTQS();
		boxTransform.translation += glm::rotate(boxTransform.rotation, scaledOffset);
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
		const Vector<glm::quat, InlineAllocator<6>> sideRotQuats =
		{
			glm::rotate(glm::quat(1, 0, 0, 0), glm::radians(-90.f), glm::vec3(0, 0, 1)), glm::rotate(glm::quat(1, 0, 0, 0), glm::radians(90.f), glm::vec3(0, 0, 1)),
			glm::quat(1, 0, 0, 0)/*identity*/, glm::rotate(glm::quat(1, 0, 0, 0), glm::radians(180.f), glm::vec3(1, 0, 0)),
			glm::rotate(glm::quat(1, 0, 0, 0), glm::radians(90.f), glm::vec3(1, 0, 0)), glm::rotate(glm::quat(1, 0, 0, 0), glm::radians(-90.f), glm::vec3(1, 0, 0))
		};

		BoxColliderComponentHitProxyContext context;
		for (int32_t i = 0; i < sideDirs.size(); i++)
		{
			const glm::vec3& locDir = sideDirs[i];
			const glm::quat& locRotQuat = sideRotQuats[i];
			context.dir = locDir;

			float minSideScale = 1.f;
			if (locDir.x != 0)
			{
				minSideScale = glm::min(boxTransform.scale.y, boxTransform.scale.z);
			}
			else if (locDir.y != 0)
			{
				minSideScale = glm::min(boxTransform.scale.x, boxTransform.scale.z);
			}
			else if (locDir.z != 0)
			{
				minSideScale = glm::min(boxTransform.scale.x, boxTransform.scale.y);
			}

			constexpr float CONE_SCALE_MULTIPLIER = 0.15f;
			//constexpr float CONE_MIN_SCALE = 0.1f;
			const float coneScale = CONE_SCALE_MULTIPLIER;//glm::max(CONE_MIN_SCALE, (minSideScale * CONE_SCALE_MULTIPLIER));
			extendSphereTransform.scale = glm::vec3(coneScale);

			constexpr float CONE_MESH_HEIGHT = 100.f;
			constexpr float CONE_PADDING_MUL = 1.f; // one full cone mesh away
			const float coneDistFromBox = coneScale * CONE_MESH_HEIGHT * CONE_PADDING_MUL;
			const glm::vec3 dir = glm::rotate(boxTransform.rotation, locDir);
			extendSphereTransform.translation = boxTransform.translation + dir * (scaledHalfSize + coneDistFromBox);
		
			extendSphereTransform.rotation = boxTransform.rotation * locRotQuat;
			editorDrawInterface.DrawMesh<BoxColliderComponentVisualizer>(
				EditorResources::GetEditorMesh(EditorMesh::Cone),
				EditorResources::GetEditorMesh(EditorMesh::Cone)->GetMaterialTable().GetMaterial(0),
				extendSphereTransform,
				context);
		}
	}

	void HandleVisProxyInteraction(Volt::BoxColliderComponent& component, Volt::Entity entity, const BoxColliderComponentHitProxyContext& context)
	{
		component.halfSize += glm::abs(context.dir) * 5.f;
		component.offset += context.dir * 5.f;
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(BoxColliderComponentVisualizer);
