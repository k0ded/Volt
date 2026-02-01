#pragma once

#include "Sandbox/ComponentVisualizers/EditorGizmoDrawer.h"

class ComponentVisualizer
{
public:
	virtual ~ComponentVisualizer() = default;

	/*
		Allows each component to have it's own gizmo. 
	*/
	virtual void DrawGizmo(EditorGizmoDrawer& gizmoDrawer) {};

private:
};
