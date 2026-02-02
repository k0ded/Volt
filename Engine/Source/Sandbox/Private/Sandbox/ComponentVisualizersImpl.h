#pragma once

#include "ComponentVisualizers/ComponentVisualizer.h"
#include "ComponentVisualizers/ComponentVisualizerRegistry.h"

#include "Sandbox/Utility/EditorResources.h"

#include <Volt-CoreComponents/LightComponents.h>
#include <Volt-CoreComponents/RenderingComponents.h>

class PointLightComponentVisualizer : public ComponentVisualizer
{
public:
	void DrawVisualization(EditorDrawInterface& gizmoDrawer)
	{
		gizmoDrawer.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::EntityGizmo));
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(PointLightComponentVisualizer, Volt::PointLightComponent);

class SpotLightComponentVisualizer : public ComponentVisualizer
{
public:
	void DrawVisualization(EditorDrawInterface& gizmoDrawer)
	{
		gizmoDrawer.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::Fill));
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(SpotLightComponentVisualizer, Volt::SpotLightComponent);

class SphereLightComponentVisualizer : public ComponentVisualizer
{
public:
	void DrawVisualization(EditorDrawInterface& gizmoDrawer)
	{
		gizmoDrawer.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::Paint));
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(SphereLightComponentVisualizer, Volt::SphereLightComponent);

class RectangleLightComponentVisualizer : public ComponentVisualizer
{
public:
	void DrawVisualization(EditorDrawInterface& gizmoDrawer)
	{
		gizmoDrawer.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::Directory));
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(RectangleLightComponentVisualizer, Volt::RectangleLightComponent);

class DirectionalLightComponentVisualizer : public ComponentVisualizer
{
public:
	void DrawVisualization(EditorDrawInterface& gizmoDrawer)
	{
		gizmoDrawer.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::LightGizmo));
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(DirectionalLightComponentVisualizer, Volt::DirectionalLightComponent);

class SkyLightComponentVisualizer : public ComponentVisualizer
{
public:
	void DrawVisualization(EditorDrawInterface& gizmoDrawer)
	{
		gizmoDrawer.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::LightGizmo));
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(SkyLightComponentVisualizer, Volt::SkylightComponent);

class CameraComponentVisualizer : public ComponentVisualizer
{
public:
	void DrawVisualization(EditorDrawInterface& gizmoDrawer)
	{
		gizmoDrawer.DrawMesh(EditorResources::GetEditorMesh(EditorMesh::Camera), EditorResources::GetEditorMesh(EditorMesh::Camera)->GetMaterialTable().GetMaterial(0));
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(CameraComponentVisualizer, Volt::CameraComponent);
