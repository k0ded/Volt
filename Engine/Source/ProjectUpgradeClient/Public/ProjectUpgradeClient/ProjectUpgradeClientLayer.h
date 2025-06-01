#pragma once

#include <Volt-Application/ApplicationLayer.h>

#include <EventSystem/EventListener.h>


namespace Volt
{
	class AppUpdateEvent;
	class AppImGuiUpdateEvent;
	class ProjectUpgradeClientLayer : public Volt::ApplicationLayer, public Volt::EventListener
	{
	public:
		ProjectUpgradeClientLayer() = default;
		~ProjectUpgradeClientLayer() override = default;

		void OnAttach() override;
		void OnDetach() override;

	private:
		bool OnUpdateEvent(Volt::AppUpdateEvent& e);
		bool OnImGuiUpdateEvent(Volt::AppImGuiUpdateEvent& e);
	};
}
