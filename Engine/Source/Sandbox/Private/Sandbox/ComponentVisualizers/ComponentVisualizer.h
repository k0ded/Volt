#pragma once

class BaseComponentVisualizer
{
public:
	virtual ~BaseComponentVisualizer() = default;
};

template<typename T>
class ComponentVisualizer : public BaseComponentVisualizer
{
public:
	using ComponentType = T;

	virtual ~ComponentVisualizer() override = default;

	/*
		Allows each component to have it's own gizmo. 
	*/
	// void DrawVisualization(EditorDrawInterface& editorDrawInterface, <ComponentType>, Volt::Entity entity)
};
