#pragma once

#include "Sandbox/ComponentVisualizers/EditorDrawInterface.h"

class ComponentVisualizer
{
public:
	virtual ~ComponentVisualizer() = default;

	/*
		Allows each component to have it's own gizmo. 
	*/
	virtual void DrawVisualization(EditorDrawInterface& gizmoDrawer) {};

private:
};
