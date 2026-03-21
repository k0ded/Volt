#pragma once

#include <Circuit/Widgets/CompoundWidget.h>

class AssetBrowserWidget : public Circuit::CompoundWidget
{
public:
	AssetBrowserWidget();
	virtual ~AssetBrowserWidget();

	CIRCUIT_BEGIN_ARGS(AssetBrowserWidget)
	{};

	CIRCUIT_END_ARGS();

	void Build(const Arguments& args);

	virtual glm::vec2 GetDesiredSize() override;
};
