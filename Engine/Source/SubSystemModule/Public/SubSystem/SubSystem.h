#pragma once

#include "SubSystem/SubSystemRegistry.h"

class SubSystemDependencyList;

class SubSystem
{
public:
	SubSystem() = default;
	virtual ~SubSystem() = default;

	/*
		Called when this subsystem should be initialized. Defined by the order of initialization stated on registration.
	*/
	virtual void Initialize() {}

	/*
		Called when this subsystem should be shutdown. Defined by the inverse order of initialization stated on registration.
	*/
	virtual void Shutdown() {}

	/*
		Called once all subsystems has been initialized.
	*/
	virtual void OnPostInitialization() {}

	/*
		SubSystems can implement
		
		SubSystemClass::GetDependencies(SubSystemDependencyList& outDependencies)

		to define it's dependencies.
	*/
};
