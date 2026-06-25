#include "ecscsbpch.h"
#include "ECSCircuitSandbox/CircuitSandbox.h"

#include <ECSCircuit/CircuitManager.h>
#include <EventSystem/ApplicationEvents.h>

CircuitSandbox::CircuitSandbox()
{
	VT_ASSERT_MSG(!s_instance, "CircuitSandbox already exists!");
	s_instance = this;
}

CircuitSandbox::~CircuitSandbox()
{
	s_instance = nullptr;
}

void CircuitSandbox::RegisterEventListeners()
{
	auto isInitializedPred = [this]() { return m_isInitialized; };

	RegisterListener<Volt::AppUpdateEvent>(VT_BIND_EVENT_FN(CircuitSandbox::OnUpdateEvent), isInitializedPred);
}

void CircuitSandbox::OnAttach()
{
	RegisterEventListeners();

	CircuitManager::Initialize();

	CircuitManager::Get().CreateWindow();

	m_isInitialized = true;
}

void CircuitSandbox::OnDetach()
{
	m_isInitialized = false;

	CircuitManager::Shutdown();

	s_instance = nullptr;
}

bool CircuitSandbox::OnUpdateEvent(Volt::AppUpdateEvent& e)
{
	VT_PROFILE_FUNCTION();

	CircuitManager::Get().Update();

	return false;
}
