#pragma once

#include "Volt/Core/Base.h"
#include "Volt/Math/Spline.h"

#include "Sandbox/Window/EditorWindow.h"

#include <Volt-Scene/Scene.h>
#include <Volt-Scene/Scene.h>

#include <glm/glm.hpp>

class SplinePanel : public EditorWindow
{
public:
	SplinePanel(Ref<Volt::Scene>& aScene);
	void UpdateMainContent() override;

private:
	Ref<Volt::Scene>& myCurrentScene;
	Ref<Spline> mySpline;

	bool mySplineCreated = false;
};
