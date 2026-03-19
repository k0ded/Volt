#pragma once

#include <Circuit/Widgets/CompoundWidget.h>

namespace Volt
{
	class SceneRenderer;
}

class ViewportWidget : public Circuit::CompoundWidget
{
public:
	ViewportWidget();
	virtual ~ViewportWidget();

	CIRCUIT_BEGIN_ARGS(ViewportWidget)
	{};

	CIRCUIT_ARGUMENT(Ref<Volt::SceneRenderer>, SceneRenderer);

	CIRCUIT_END_ARGS();

	void Build(const Arguments& args);

	virtual glm::vec2 GetDesiredSize() override;

	virtual void OnPaint(Circuit::CircuitPainter& painter) override;
private:
	Ref<Volt::SceneRenderer> m_sceneRenderer;
};
