#pragma once

#include <CoreUtilities/CompilerTraits.h>

namespace Volt
{
	class FileHandle
	{
	public:
		FileHandle()
			: m_handle(nullptr)
		{}

		FileHandle(void* inHandle)
			: m_handle(inHandle)
		{}

		FileHandle(const FileHandle& other) noexcept
		{
			m_handle = other.m_handle;
		}

		FileHandle& operator=(const FileHandle& other) noexcept
		{
			if (&other != this)
			{
				m_handle = other.m_handle;
			}

			return *this;
		}

		FileHandle(FileHandle&& other) noexcept
			: m_handle(other.m_handle)
		{
			other.m_handle = nullptr;
		}

		FileHandle& operator=(FileHandle&& other) noexcept
		{
			if (&other != this)
			{
				m_handle = other.m_handle;
				other.m_handle = nullptr;
			}
		
			return *this;
		}

		VT_NODISCARD VT_INLINE bool IsValid() const { return m_handle != nullptr; }
		VT_NODISCARD VT_INLINE void* Get() const { return m_handle; }
		VT_INLINE void Reset() { m_handle = nullptr; }

	private:
		void* m_handle;
	};
}
