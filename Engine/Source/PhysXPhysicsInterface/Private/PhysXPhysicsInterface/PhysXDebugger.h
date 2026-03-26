#pragma once

#include <CoreUtilities/Filesystem/Path.h>

namespace physx
{
	class PxPvd;
	class PxPvdTransport;
	class PxFoundation;
}

namespace Volt
{
	class PhysXDebugger
	{
	public:
		PhysXDebugger(physx::PxFoundation& foundation);
		~PhysXDebugger();

		void StartDebugging(const Filesystem::Path& path, bool networkDebug = false);
		void StopDebugging();

		bool IsDebugging() const;

		VT_NODISCARD VT_INLINE physx::PxPvd* GetPvdInstance() const { return m_debugger; }

	private:
		physx::PxPvd* m_debugger = nullptr;
		physx::PxPvdTransport* m_debuggingTransport = nullptr;
	};
}
