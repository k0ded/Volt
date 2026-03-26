#include "Volt-FileSystem/Iterators/DirectoryIterator.h"

namespace Filesystem
{
	DirectoryIterator::DirectoryIterator(const Path& rootPath)
	{
		m_platformIterator = CreateRef<Volt::PlatformDirectoryIterator>(rootPath);
		Advance();
	}

	void DirectoryIterator::Advance()
	{
		m_isActive = m_platformIterator->Advance(m_activeEntry.path, m_activeEntry.isDirectory);
		if (!m_isActive)
		{
			m_platformIterator = nullptr;
		}
	}
}
