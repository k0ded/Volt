#pragma once
#include <Volt-Application/ApplicationLayer.h>

#include <EventSystem/EventListener.h>

namespace Volt
{
	class AppImGuiUpdateEvent;
}

class ProjectUpgradeLayer : public Volt::ApplicationLayer, public Volt::EventListener
{
public:
	ProjectUpgradeLayer();
	~ProjectUpgradeLayer() override;

	void OnAttach() override;
	void OnDetach() override;

private:
	void DrawUpgradeUI();
	bool OnImGuiUpdateEvent(Volt::AppImGuiUpdateEvent& e);
};
