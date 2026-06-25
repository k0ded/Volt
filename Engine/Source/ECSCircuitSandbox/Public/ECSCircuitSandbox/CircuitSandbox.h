#pragma once

#include <Volt-Application/ApplicationLayer.h>

#include <EventSystem/EventListener.h>

namespace Volt
{
	class Event;
	class AppUpdateEvent;
}

class CircuitSandbox : public Volt::ApplicationLayer, public Volt::EventListener
{
public:
	CircuitSandbox();
	~CircuitSandbox() override;

	void OnAttach() override;
	void OnDetach() override;

	VT_NODISCARD VT_INLINE static CircuitSandbox& Get() { return *s_instance; }
private:
	void RegisterEventListeners();

	bool OnUpdateEvent(Volt::AppUpdateEvent& e);

	bool m_isInitialized = false;

	inline static CircuitSandbox* s_instance = nullptr;
};
