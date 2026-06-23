#include "ecscircuitpch.h"

#include "ECSCircuit/CircuitManager.h"
#include "ECSCircuit/LogCategories.h"

#include <WindowModule/WindowManager_New.h>
#include <WindowModule/Window_New.h>

#include <CoreUtilities/VoltAssert.h>

Unique<CircuitManager> CircuitManager::s_instance = nullptr;

CircuitManager::CircuitManager()
{}

CircuitManager& CircuitManager::Get()
{
	return *s_instance;
}

void CircuitManager::Initialize()
{
	VT_ASSERT_MSG(!s_instance, "CircuitManager already initialized!");
	s_instance = CreateUnique<CircuitManager>();
	VT_LOGC(Info, LogECSCircuit, "CircuitManager initialized.");
}

void CircuitManager::Shutdown()
{
	s_instance = nullptr;
}

void CircuitManager::Update()
{}

void CircuitManager::CreateWindow()
{
	Volt::WindowInitializer windowInitializer{};
	windowInitializer.initialWidth = 1600;
	windowInitializer.initialHeight = 900;
	windowInitializer.initialPosX = 0;
	windowInitializer.initialPosY = 0;
	windowInitializer.enableVSync = true;
	windowInitializer.createAsDecorated = true;

	Volt::WindowHandle windowHandle = Volt::WindowManager_New::Get().CreateWindow(windowInitializer);
	m_windows[windowHandle] = CreateRef<CircuitWindowStub>();
}
