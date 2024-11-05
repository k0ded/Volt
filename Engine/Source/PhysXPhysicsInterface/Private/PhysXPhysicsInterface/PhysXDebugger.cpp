#include "pxpch.h"

#include "PhysXPhysicsInterface/PhysXDebugger.h"

#include <PhysX/PxPhysicsAPI.h>

namespace Volt
{
	PhysXDebugger::PhysXDebugger(physx::PxFoundation& foundation)
	{
		m_debugger = PxCreatePvd(foundation);
		VT_ASSERT_MSG(m_debugger, "PxCreatePvd failed!");
	}

	PhysXDebugger::~PhysXDebugger()
	{
		StopDebugging();

		if (m_debugger)
		{
			m_debugger->release();
		}

		m_debugger = nullptr;
	}
	
	void PhysXDebugger::StartDebugging(const std::filesystem::path& path, bool networkDebug /* = false */)
	{
		VT_ENSURE(m_debugger);

		StopDebugging();

		if (!networkDebug)
		{
			m_debuggingTransport = physx::PxDefaultPvdFileTransportCreate((path.string() + ".pxd2").c_str());
			m_debugger->connect(*m_debuggingTransport, physx::PxPvdInstrumentationFlag::eALL);
		}
		else
		{
			m_debuggingTransport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
			m_debugger->connect(*m_debuggingTransport, physx::PxPvdInstrumentationFlag::eALL);
		}
	}

	void PhysXDebugger::StopDebugging()
	{
		VT_ENSURE(m_debugger);
	
		if (!IsDebugging())
		{
			return;
		}

		m_debugger->disconnect();
		m_debuggingTransport->release();
		m_debuggingTransport = nullptr;
	}

	bool PhysXDebugger::IsDebugging() const
	{
		VT_ENSURE(m_debugger);
		return m_debugger->isConnected();
	}
}
