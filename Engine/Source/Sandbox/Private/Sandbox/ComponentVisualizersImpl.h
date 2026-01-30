#pragma once

#include "ComponentVisualizers/ComponentVisualizer.h"
#include "ComponentVisualizers/ComponentVisualizerRegistry.h"

#include "Sandbox/Utility/EditorResources.h"

#include <Volt-CoreComponents/LightComponents.h>

class PointLightComponentVisualizer : public ComponentVisualizer
{
public:
	RefPtr<Volt::RHI::Image> GetIcon() const override
	{
		return EditorResources::GetEditorIcon(EditorIcon::LightGizmo);
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(PointLightComponentVisualizer, Volt::PointLightComponent);

class SpotLightComponentVisualizer : public ComponentVisualizer
{
public:
	RefPtr<Volt::RHI::Image> GetIcon() const override
	{
		return EditorResources::GetEditorIcon(EditorIcon::LightGizmo);
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(SpotLightComponentVisualizer, Volt::SpotLightComponent);

class SphereLightComponentVisualizer : public ComponentVisualizer
{
public:
	RefPtr<Volt::RHI::Image> GetIcon() const override
	{
		return EditorResources::GetEditorIcon(EditorIcon::LightGizmo);
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(SphereLightComponentVisualizer, Volt::SphereLightComponent);

class RectangleLightComponentVisualizer : public ComponentVisualizer
{
public:
	RefPtr<Volt::RHI::Image> GetIcon() const override
	{
		return EditorResources::GetEditorIcon(EditorIcon::LightGizmo);
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(RectangleLightComponentVisualizer, Volt::RectangleLightComponent);

class DirectionalLightComponentVisualizer : public ComponentVisualizer
{
public:
	RefPtr<Volt::RHI::Image> GetIcon() const override
	{
		return EditorResources::GetEditorIcon(EditorIcon::LightGizmo);
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(DirectionalLightComponentVisualizer, Volt::DirectionalLightComponent);

class SkyLightComponentVisualizer : public ComponentVisualizer
{
public:
	RefPtr<Volt::RHI::Image> GetIcon() const override
	{
		return EditorResources::GetEditorIcon(EditorIcon::LightGizmo);
	}
};
VT_REGISTER_COMPONENT_VISUALIZER(SkyLightComponentVisualizer, Volt::SkylightComponent);
