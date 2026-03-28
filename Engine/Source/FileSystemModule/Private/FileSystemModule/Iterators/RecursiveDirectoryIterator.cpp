#include "FileSystemModule/Iterators/RecursiveDirectoryIterator.h"

namespace Filesystem
{
	RecursiveDirectoryIterator::RecursiveDirectoryIterator(const Path& rootPath)
	{
		m_platformIterator = CreateRef<Volt::PlatformRecursiveDirectoryIterator>(rootPath);
		Advance();
	}

	void RecursiveDirectoryIterator::Advance()
	{
		m_isActive = m_platformIterator->Advance(m_activeEntry.path, m_activeEntry.isDirectory);
		if (!m_isActive)
		{
			m_platformIterator = nullptr;
		}
	}
}
