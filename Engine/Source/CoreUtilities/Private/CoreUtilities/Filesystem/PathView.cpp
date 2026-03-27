#include "cupch.h"

#include "CoreUtilities/Filesystem/PathView.h"
#include "CoreUtilities/Filesystem/Path.h"
#include "CoreUtilities/Filesystem/PathHelpers.h"

#include <filesystem>

namespace Filesystem
{
	PathView::PathView(const WStringView pathView)
		: m_stringView(pathView)
	{

	}

	bool operator==(const PathView& lhs, const wchar_t* rhs) noexcept
	{
		return lhs.Compare(WStringView(rhs)) == 0;
	}

	bool operator==(const PathView& lhs, const PathView& rhs) noexcept
	{
		return lhs.Compare(rhs) == 0;
	}

	bool operator==(const PathView& lhs, const WStringView& rhs) noexcept
	{
		return lhs.Compare(rhs) == 0;
	}

	std::strong_ordering operator<=>(const PathView& lhs, const PathView& rhs) noexcept
	{
		return lhs.Compare(rhs) <=> 0;
	}

	const wchar_t* PathView::CStr() const
	{
		return m_stringView.data();
	}

	int32_t PathView::Compare(const WStringView other) const
	{
		std::filesystem::path tempThis(m_stringView.begin(), m_stringView.end());
		std::wstring_view tempOther(other.begin(), other.size());

		return tempThis.compare(tempOther);
	}

	int32_t PathView::Compare(const PathView& other) const
	{
		return Compare(WStringView(other.m_stringView));
	}

	PathView PathView::RootName() const
	{
		const Helpers::RootParts rootParts = Helpers::ParseRoot(m_stringView);
		return rootParts.rootName;
	}
	
	PathView PathView::RootDirectory() const
	{
		const Helpers::RootParts rootParts = Helpers::ParseRoot(m_stringView);
		return rootParts.rootDirectory;
	}
	
	PathView PathView::RootPath() const
	{
		const Helpers::RootParts rootParts = Helpers::ParseRoot(m_stringView);
		return WStringView(rootParts.rootName.begin(), rootParts.rootName.size() + rootParts.rootDirectory.size());
	}
	
	PathView PathView::RelativePath() const
	{
		const Helpers::RootParts rootParts = Helpers::ParseRoot(m_stringView);

		const wchar_t* begin = m_stringView.data() + rootParts.pathStart;
		size_t length = m_stringView.size() - rootParts.pathStart;

		return WStringView(begin, length);
	}
	
	PathView PathView::ParentPath() const
	{
		const Helpers::PathParts pathParts = Helpers::ParsePath(m_stringView);
		return pathParts.parent;
	}
	
	PathView PathView::Filename() const
	{
		const Helpers::PathParts pathParts = Helpers::ParsePath(m_stringView);
		return pathParts.filename;
	}
	
	PathView PathView::Stem() const
	{
		const Helpers::PathParts pathParts = Helpers::ParsePath(m_stringView);
		return pathParts.stem;
	}
	
	PathView PathView::Extension() const
	{
		const Helpers::PathParts pathParts = Helpers::ParsePath(m_stringView);
		return pathParts.extension;
	}

	bool PathView::IsEmpty() const
	{
		return m_stringView.empty();
	}

	bool PathView::HasRootPath() const
	{
		return RootPath().IsEmpty();
	}

	bool PathView::HasRootName() const
	{
		return RootName().IsEmpty();
	}

	bool PathView::HasRootDirectory() const
	{
		return RootDirectory().IsEmpty();
	}

	bool PathView::HasRelativePath() const
	{
		return RelativePath().IsEmpty();
	}

	bool PathView::HasParentPath() const
	{
		return ParentPath().IsEmpty();
	}

	bool PathView::HasFilename() const
	{
		return Filename().IsEmpty();
	}

	bool PathView::HasStem() const
	{
		return Stem().IsEmpty();
	}

	bool PathView::HasExtension() const
	{
		return Extension().IsEmpty();
	}

	bool PathView::IsAbsolute() const
	{
		const Helpers::RootParts rootParts = Helpers::ParseRoot(m_stringView);

		if (rootParts.rootDirectory.empty())
		{
			return false;
		}

		if (!rootParts.rootName.empty())
		{
			return true;
		}

		return false;
	}

	bool PathView::IsRelative() const
	{
		return !IsAbsolute();
	}
}
