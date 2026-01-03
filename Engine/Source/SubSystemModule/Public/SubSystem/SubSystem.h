#pragma once

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
		Called once all subsystems in the current initialization stage has been initialized.
		Will be called in the initialization order.
	*/
	virtual void OnPostStageInitializaton() {}

	/*
		Called once all subsystems has been initialized.
	*/
	virtual void OnPostInitialization() {}

	/*
		Called before subsystems begin shutting down.
	*/
	virtual void OnPreShutdown() {}

	/*
		SubSystems can implement
		
		static void SubSystemClass::GetSubSystemDependencies(SubSystemDependencyList& outDependencies)

		to define it's dependencies.
	*/
};
