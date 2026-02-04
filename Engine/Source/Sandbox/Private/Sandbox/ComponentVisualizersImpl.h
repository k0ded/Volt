#pragma once

#include "ComponentVisualizers/ComponentVisualizer.h"
#include "ComponentVisualizers/ComponentVisualizerRegistry.h"

#include "Sandbox/Utility/EditorResources.h"

#include <Volt-CoreComponents/LightComponents.h>
#include <Volt-CoreComponents/RenderingComponents.h>

struct HitProxyContext
{
	float a;
	float b;
};

class PointLightComponentVisualizer : public ComponentVisualizer<Volt::PointLightComponent>
{
public:
	void DrawVisualization(EditorDrawInterface& editorDrawInterface)
	{
		HitProxyContext hitProxyContext;
		hitProxyContext.a = 1.f;
		hitProxyContext.b = 10.f;

		//editorDrawInterface.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::EntityGizmo), hitProxyContext);
	}

	void DrawVisualization(EditorDrawInterface& editorDrawInterface, Volt::PointLightComponent& component, Volt::Entity entity)
	{
		editorDrawInterface.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::LightGizmo));
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(PointLightComponentVisualizer);

class SpotLightComponentVisualizer : public ComponentVisualizer<Volt::SpotLightComponent>
{
public:
	void DrawVisualization(EditorDrawInterface& editorDrawInterface, Volt::SpotLightComponent& component, Volt::Entity entity)
	{
		editorDrawInterface.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::LightGizmo));
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(SpotLightComponentVisualizer);

class SphereLightComponentVisualizer : public ComponentVisualizer<Volt::SphereLightComponent>
{
public:
	void DrawVisualization(EditorDrawInterface& editorDrawInterface, Volt::SphereLightComponent& component, Volt::Entity entity)
	{
		editorDrawInterface.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::LightGizmo));
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(SphereLightComponentVisualizer);

class RectangleLightComponentVisualizer : public ComponentVisualizer<Volt::RectangleLightComponent>
{
public:
	void DrawVisualization(EditorDrawInterface& editorDrawInterface, Volt::RectangleLightComponent& component, Volt::Entity entity)
	{
		editorDrawInterface.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::LightGizmo));
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(RectangleLightComponentVisualizer);

class DirectionalLightComponentVisualizer : public ComponentVisualizer<Volt::DirectionalLightComponent>
{
public:
	void DrawVisualization(EditorDrawInterface& editorDrawInterface, Volt::DirectionalLightComponent& component, Volt::Entity entity)
	{
		editorDrawInterface.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::LightGizmo));
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(DirectionalLightComponentVisualizer);

class SkyLightComponentVisualizer : public ComponentVisualizer<Volt::SkylightComponent>
{
public:
	void DrawVisualization(EditorDrawInterface& editorDrawInterface, Volt::SkylightComponent& component, Volt::Entity entity)
	{
		editorDrawInterface.DrawIcon(EditorResources::GetEditorIcon(EditorIcon::LightGizmo));
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(SkyLightComponentVisualizer);

class CameraComponentVisualizer : public ComponentVisualizer<Volt::CameraComponent>
{
public:
	void DrawVisualization(EditorDrawInterface& editorDrawInterface, Volt::CameraComponent& component, Volt::Entity entity)
	{
		HitProxyContext testContext;
		testContext.a = 10.f;
		testContext.b = 5.f;

		editorDrawInterface.DrawMesh<CameraComponentVisualizer>(EditorResources::GetEditorMesh(EditorMesh::Camera), EditorResources::GetEditorMesh(EditorMesh::Camera)->GetMaterialTable().GetMaterial(0), testContext);
	}

	void HandleVisProxyInteraction(const Volt::CameraComponent& component, Volt::Entity entity, const HitProxyContext& context)
	{
		//VT_DEBUGBREAK();
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(CameraComponentVisualizer);
