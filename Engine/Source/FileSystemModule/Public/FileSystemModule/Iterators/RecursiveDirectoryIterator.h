#pragma once

#include "FileSystemModule/Config.h"

#include <PlatformsModule/Platform.h>

#include <CoreUtilities/Filesystem/Path.h>
#include <CoreUtilities/Pointers/Ref.h>

namespace Filesystem
{
	class RecursiveDirectoryIterator
	{
	public:
		struct Entry
		{
			Filesystem::Path path;
			bool isDirectory;
		};
		
		RecursiveDirectoryIterator() = default;
		VTFS_API RecursiveDirectoryIterator(const Path& rootPath);

		VT_INLINE const Entry& operator*() const { return m_activeEntry; }
		VT_INLINE const Entry* operator->() const { return &m_activeEntry; }
		VT_INLINE RecursiveDirectoryIterator& operator++() { Advance(); return *this; }

		VT_INLINE explicit operator bool() const { return m_isActive; }
		VT_INLINE bool operator!=(const RecursiveDirectoryIterator& other) const { return m_platformIterator != other.m_platformIterator; }

		VT_INLINE RecursiveDirectoryIterator begin() { return *this; }
		VT_INLINE RecursiveDirectoryIterator end() { return {}; }

	private:
		VTFS_API void Advance();

		Entry m_activeEntry;
		bool m_isActive = false;

		Ref<Volt::PlatformRecursiveDirectoryIterator> m_platformIterator;
	};
}
