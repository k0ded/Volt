#include "pxpch.h"

#include "PhysXPhysicsInterface/PhysXPhysicsCore.h"
#include "PhysXPhysicsInterface/PhysXDebugger.h"
#include "PhysXPhysicsInterface/PhysXContactListener.h"
#include "PhysXPhysicsInterface/PhysXPhysicsScene.h"
#include "PhysXPhysicsInterface/PhysXPhysicsMaterial.h"

#include <PhysX/PxPhysicsAPI.h>

namespace Volt
{
	class PhysicsErrorCallback : public physx::PxErrorCallback
	{
	public:
		void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override
		{
			const char* errorMessage = nullptr;

			switch (code)
			{
				case physx::PxErrorCode::eNO_ERROR:				errorMessage = "No Error"; break;
				case physx::PxErrorCode::eDEBUG_INFO:			errorMessage = "Info"; break;
				case physx::PxErrorCode::eDEBUG_WARNING:		errorMessage = "Warning"; break;
				case physx::PxErrorCode::eINVALID_PARAMETER:	errorMessage = "Invalid Parameter"; break;
				case physx::PxErrorCode::eINVALID_OPERATION:	errorMessage = "Invalid Operation"; break;
				case physx::PxErrorCode::eOUT_OF_MEMORY:		errorMessage = "Out Of Memory"; break;
				case physx::PxErrorCode::eINTERNAL_ERROR:		errorMessage = "Internal Error"; break;
				case physx::PxErrorCode::eABORT:				errorMessage = "Abort"; break;
				case physx::PxErrorCode::ePERF_WARNING:			errorMessage = "Performance Warning"; break;
				case physx::PxErrorCode::eMASK_ALL:				errorMessage = "Unknown Error"; break;
			}

			switch (code)
			{
				case physx::PxErrorCode::eNO_ERROR:
				case physx::PxErrorCode::eDEBUG_INFO:
					VT_LOGC(Info, LogPhysX, "[PhysX]: {0}: {1} at {2} ({3})", errorMessage, message, file, line);
					break;
				case physx::PxErrorCode::eDEBUG_WARNING:
				case physx::PxErrorCode::ePERF_WARNING:
					VT_LOGC(Info, LogPhysX, "[PhysX]: {0}: {1} at {2} ({3})", errorMessage, message, file, line);
					break;
				case physx::PxErrorCode::eINVALID_PARAMETER:
				case physx::PxErrorCode::eINVALID_OPERATION:
				case physx::PxErrorCode::eOUT_OF_MEMORY:
				case physx::PxErrorCode::eINTERNAL_ERROR:
					VT_LOGC(Error, LogPhysX, "[PhysX]: {0}: {1} at {2} ({3})", errorMessage, message, file, line);
					break;
				case physx::PxErrorCode::eABORT:
				case physx::PxErrorCode::eMASK_ALL:
					VT_LOGC(Critical, LogPhysX, "[PhysX]: {0}: {1} at {2} ({3})", errorMessage, message, file, line);
					VT_ASSERT(false);
					break;
			}
		}
	};

	class PhysicsAssertHandler : public physx::PxAssertHandler
	{
		void operator()(const char* exp, const char* file, int line, bool& ignore) override
		{
			VT_LOGC(Critical, LogPhysX, "[PhysX Error]: {0}:{1} - {2}", file, line, exp);
		}
	};

	PhysicsErrorCallback g_physicsErrorCallback;
#ifdef VT_ENABLE_ASSERTS
	PhysicsAssertHandler g_physicsAssertHandler;
#endif

	PhysXPhysicsCore::PhysXPhysicsCore(const PhysicsCoreCreateInfo& createInfo)
		: m_contactListener(createInfo.contactListener)
	{
		VT_ENSURE(!s_instance);
		s_instance = this;

		m_physXAllocator = CreateScope<physx::PxDefaultAllocator>();

		m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, *m_physXAllocator, g_physicsErrorCallback);
		VT_ASSERT_MSG(m_foundation, "PxCreateFoundation failed!");

		m_physXDebugger = CreateScope<PhysXDebugger>(*m_foundation);

		physx::PxTolerancesScale tolerances{};
		tolerances.length = 100;
		tolerances.speed = 1000;

		m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, tolerances, createInfo.allowDebugging, m_physXDebugger->GetPvdInstance());
		VT_ASSERT_MSG(m_physics, "PxCreatePhysics failed!");

		VT_CHECK_MSG(PxInitExtensions(*m_physics, m_physXDebugger->GetPvdInstance()), "Failed to initialize PhysX extensions!");
	
		m_defaultCPUDispatcher = physx::PxDefaultCpuDispatcherCreate(4);

#ifdef VT_ENABLE_ASSERTS
		PxSetAssertHandler(g_physicsAssertHandler);
#endif

		m_physXContactListener = CreateScope<PhysXContactListener>(m_contactListener);
	}

	PhysXPhysicsCore::~PhysXPhysicsCore()
	{
		m_defaultCPUDispatcher->release();
		m_defaultCPUDispatcher = nullptr;

		m_physXContactListener = nullptr;

		PxCloseExtensions();

		m_physXDebugger->StopDebugging();

		m_physics->release();
		m_physics = nullptr;

		m_physXDebugger = nullptr;

		s_instance = nullptr;
	}

	Ref<PhysicsScene> PhysXPhysicsCore::CreateScene(const PhysicsSceneCreateInfo& createInfo) const
	{
		return CreateRef<PhysXPhysicsScene>(createInfo);
	}

	Ref<PhysicsMaterial> PhysXPhysicsCore::CreateMaterial(const PhysicsMaterialCreateInfo& createInfo) const
	{
		return CreateRef<PhysXPhysicsMaterial>(createInfo);
	}
}

