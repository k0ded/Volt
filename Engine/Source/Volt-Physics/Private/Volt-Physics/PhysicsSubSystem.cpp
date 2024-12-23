#include "vppch.h"

#include "Volt-Physics/PhysicsSubSystem.h"

#include <Volt-Core/DynamicLibraryManager.h>

#include <CoreUtilities/DynamicLibraryHelpers.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(PhysicsSubSystem, Engine, 0);

	void PhysicsSubSystem::Initialize()
	{
		LoadPhysicsInterface();

		PhysicsCoreCreateInfo coreCreateInfo{};
		coreCreateInfo.allowDebugging = true;

		m_physicsCore = m_physicsCoreCreateFunc(coreCreateInfo);
	}

	void PhysicsSubSystem::Shutdown()
	{
		if (m_physicsCore)
		{
			m_physicsCoreDestroyFunc(m_physicsCore);
			m_physicsCore = nullptr;
		}
	}

	PhysicsCore* PhysicsSubSystem::GetPhysicsCore() const
	{
		return m_physicsCore;
	}

	void PhysicsSubSystem::LoadPhysicsInterface()
	{
		bool externallyLoaded;
		DLLHandle libHandle = DynamicLibraryManager::Get().LoadDynamicLibrary(std::filesystem::absolute("Binaries\\PhysXPhysicsInterface.dll"), externallyLoaded);
		if (libHandle == nullptr)
		{
			return;
		}

		m_physicsCoreCreateFunc = reinterpret_cast<PFN_CreatePhysicsCore>(VT_GET_PROC_ADDRESS(libHandle, PHYSICS_CREATE_CORE_FUNC_NAME));
		m_physicsCoreDestroyFunc = reinterpret_cast<PFN_DestroyPhysicsCore>(VT_GET_PROC_ADDRESS(libHandle, PHYSICS_DESTROY_CORE_FUNC_NAME));
	}
}
