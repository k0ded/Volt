#pragma once

#include <CoreUtilities/CompilerTraits.h>

namespace Volt
{
	class ProcessHandle
	{
	public:
		ProcessHandle()
			: m_handle(nullptr)
		{ }

		ProcessHandle(void* inHandle)
			: m_handle(inHandle)
		{ }

		VT_NODISCARD VT_INLINE bool IsValid() const { return m_handle != nullptr; }
		VT_NODISCARD VT_INLINE void* Get() const { return m_handle; }
		VT_INLINE void Reset() { m_handle = nullptr; }

	private:
		void* m_handle;
	};
}
