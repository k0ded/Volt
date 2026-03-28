#pragma once

#include "FileSystemModule/Config.h"

#include <PlatformsModule/Platform.h>

#include <CoreUtilities/Filesystem/Path.h>
#include <CoreUtilities/Pointers/Ref.h>

namespace Filesystem
{
	class DirectoryIterator
	{
	public:
		struct Entry
		{
			Filesystem::Path path;
			bool isDirectory;
		};

		DirectoryIterator() = default;
		VTFS_API DirectoryIterator(const Path& rootPath);

		VT_INLINE const Entry& operator*() const { return m_activeEntry; }
		VT_INLINE const Entry* operator->() const { return &m_activeEntry; }
		VT_INLINE DirectoryIterator& operator++() { Advance(); return *this; }

		VT_INLINE explicit operator bool() const { return m_isActive; }
		VT_INLINE bool operator!=(const DirectoryIterator& other) const { return m_platformIterator != other.m_platformIterator; }

		VT_INLINE DirectoryIterator begin() { return *this; }
		VT_INLINE DirectoryIterator end() { return {}; }

	private:
		VTFS_API void Advance();

		Entry m_activeEntry;
		bool m_isActive = false;

		Ref<Volt::PlatformDirectoryIterator> m_platformIterator;
	};
}
