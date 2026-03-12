#pragma once

#include "Volt-Scene/Scene.h"

#include <EventSystem/Event.h>

#include <AssetSystem/AssetReference.h>

namespace Volt
{
	class OnScenePlayEvent : public Event
	{
	public:
		OnScenePlayEvent() = default;

		EVENT_CLASS(OnScenePlayEvent, "{DDB25ECF-2095-4A95-AF48-C3101EA6756D}"_guid);
	};

	class OnSceneStopEvent : public Event
	{
	public:
		OnSceneStopEvent() = default;

		EVENT_CLASS(OnSceneStopEvent, "{BA8D9CF7-7AF9-465E-ACF7-58205BF1C21D}"_guid);
	};

	class OnSceneLoadedEvent : public Event
	{
	public:
		OnSceneLoadedEvent(AssetReference<Volt::Scene> aScene)
			: m_scene(aScene)
		{
		}

		inline AssetReference<Volt::Scene> GetScene() const { return m_scene; }

		EVENT_CLASS(OnSceneLoadedEvent, "{D8C1545C-373E-4D0D-A546-9D382ED3AE3C}"_guid);

	private:
		AssetReference<Volt::Scene> m_scene;
	};
}
