#include "csbpch.h"
#include "CircuitSandbox.h"

#include <InputModule/Input.h>
#include <InputModule/InputCodes.h>
#include <InputModule/Events/KeyboardEvents.h>

#include <SubSystem/SubSystemManager.h>

#include <WindowModule/Events/WindowEvents.h>
#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

#include <Circuit/CircuitManager.h>
#include <Circuit/Widgets/SliderWidget.h>

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
	RegisterListener<Volt::AppRenderEvent>(VT_BIND_EVENT_FN(CircuitSandbox::OnRenderEvent), isInitializedPred);
	RegisterListener<Volt::KeyPressedEvent>(VT_BIND_EVENT_FN(CircuitSandbox::OnKeyPressedEvent), isInitializedPred);
}


void CircuitSandbox::OnAttach()
{
	RegisterEventListeners();

	//Volt::WindowManager::Get().GetMainWindow().Maximize();

	Circuit::CircuitManager::Initialize();
	Volt::WindowManager::Get().GetMainWindow().Resize(500, 300);


	m_isInitialized = true;
}

void CircuitSandbox::OnDetach()
{
	m_isInitialized = false;

	s_instance = nullptr;
}

bool CircuitSandbox::OnUpdateEvent(Volt::AppUpdateEvent& e)
{
	VT_PROFILE_FUNCTION();

	return false;
}

bool CircuitSandbox::OnRenderEvent(Volt::AppRenderEvent& e)
{
	VT_PROFILE_FUNCTION();

	return false;
}

bool CircuitSandbox::OnKeyPressedEvent(Volt::KeyPressedEvent& e)
{

	return false;
}
