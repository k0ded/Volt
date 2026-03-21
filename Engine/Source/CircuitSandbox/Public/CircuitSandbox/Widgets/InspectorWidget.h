#pragma once

#include <Circuit/Widgets/CompoundWidget.h>

class InspectorWidget : public Circuit::CompoundWidget
{
public:
	InspectorWidget();
	virtual ~InspectorWidget();

	CIRCUIT_BEGIN_ARGS(InspectorWidget)
	{};

	CIRCUIT_END_ARGS();

	void Build(const Arguments& args);

	virtual glm::vec2 GetDesiredSize() override;
};
