#pragma once

#include <Circuit/Widgets/CompoundWidget.h>

class SceneViewWidget : public Circuit::CompoundWidget
{
public:
	SceneViewWidget();
	virtual ~SceneViewWidget();

	CIRCUIT_BEGIN_ARGS(SceneViewWidget)
	{};

	CIRCUIT_END_ARGS();

	void Build(const Arguments& args);

	virtual glm::vec2 GetDesiredSize() override;
};
