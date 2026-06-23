#pragma once

#include "ECSCircuit/Config.h"

#include <WindowModule/WindowHandle.h>

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Pointers/Unique.h>
#include <CoreUtilities/Pointers/Ref.h>

struct CircuitWindowStub
{
	bool yee;
};

class CircuitManager
{
public:
	CircuitManager();
	~CircuitManager() = default;

	ECSCIRCUIT_API static CircuitManager& Get();
	ECSCIRCUIT_API static void Initialize();
	ECSCIRCUIT_API static void Shutdown();

	ECSCIRCUIT_API void Update();

	ECSCIRCUIT_API void CreateWindow();

private:
	static Unique<CircuitManager> s_instance;
	Map<Volt::WindowHandle, Ref<CircuitWindowStub>> m_windows;
};
