#include "vppch.h"

#include "Volt-Physics/PhysicsSubSystem.h"

#include <Volt-FileSystem/Filesystem.h>
#include <Volt-Core/DynamicLibraryManager.h>

#include <PhysicsInterface/PhysicsLayerManager.h>
#include <CoreUtilities/DynamicLibraryHelpers.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(PhysicsSubSystem, Default, Engine);

	PhysicsSubSystem::PhysicsSubSystem()
		: m_physicsModulePath(Filesystem::Absolute("Binaries\\PhysXPhysicsInterface.dll"))
	{
	}

	void PhysicsSubSystem::Initialize()
	{
		bool initialized = LoadPhysicsInterface();
		if (initialized)
		{
			InitializePhysicsLayers();

			PhysicsCoreCreateInfo coreCreateInfo{};
			coreCreateInfo.allowDebugging = true;

			m_physicsCore = m_physicsCoreCreateFunc(coreCreateInfo);

			VT_LOGC(Info, LogVoltPhysics, "Successfully initialized Physics SubSystem!");
		}
		else
		{
			VT_LOGC(Error, LogVoltPhysics, "Failed to initialize Physics SubSystem");
		}
	}

	void PhysicsSubSystem::Shutdown()
	{
		if (m_physicsCore)
		{
			m_physicsCoreDestroyFunc(m_physicsCore);
			m_physicsCore = nullptr;

			DynamicLibraryManager::Get().UnloadDynamicLibrary(m_physicsModulePath);
		}
	}

	PhysicsCore* PhysicsSubSystem::GetPhysicsCore() const
	{
		return m_physicsCore;
	}

	bool PhysicsSubSystem::LoadPhysicsInterface()
	{
		bool externallyLoaded;
		DLLHandle libHandle = DynamicLibraryManager::Get().LoadDynamicLibrary(m_physicsModulePath, externallyLoaded);
		if (libHandle == nullptr)
		{
			return false;
		}

		m_physicsCoreCreateFunc = reinterpret_cast<PFN_CreatePhysicsCore>(VT_GET_PROC_ADDRESS(libHandle, PHYSICS_CREATE_CORE_FUNC_NAME));
		m_physicsCoreDestroyFunc = reinterpret_cast<PFN_DestroyPhysicsCore>(VT_GET_PROC_ADDRESS(libHandle, PHYSICS_DESTROY_CORE_FUNC_NAME));
	
		return true;
	}

	void PhysicsSubSystem::InitializePhysicsLayers()
	{
		g_physicsLayerManager.AddLayer("Default");
	}
}
