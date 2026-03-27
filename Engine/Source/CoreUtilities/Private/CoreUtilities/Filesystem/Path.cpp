#include "cupch.h"

#include "CoreUtilities/Filesystem/Path.h"
#include "CoreUtilities/Filesystem/PathHelpers.h"
#include "CoreUtilities/Containers/Vector.h"

#include <filesystem>

namespace Filesystem
{
	Path::Path()
	{}

	Path::Path(WStringView string)
	{
		m_path = WString(string);
	}

	Path::Path(const WString& string)
		: m_path(string)
	{
	}

	Path::Path(StringView string)
	{
		m_path.assign_convert(string.data(), string.length());
	}

	Path::Path(const String& string)
		: m_path(WString::CtorConvert(), string)
	{

	}

	Path::Path(const WString::value_type* value)
		: m_path(value)
	{
	}

	Path::Path(const String::value_type* value)
		: m_path(WString::CtorConvert(), value)
	{
	}

	Path::~Path()
	{}

	Path::Path(const Path& other)
		: m_path(other.m_path)
	{}
	
	Path& Path::operator=(const Path& other)
	{
		if (&other != this)
		{
			m_path = other.m_path;
		}

		return *this;
	}

	Path::Path(Path&& other) noexcept
		: m_path(std::move(other.m_path))
	{}

	Path& Path::operator=(Path&& other) noexcept
	{
		if (&other != this)
		{
			m_path = std::move(other.m_path);
		}

		return *this;
	}

	bool operator==(const Path& lhs, const Path& rhs) noexcept
	{
		return lhs.Compare(rhs) == 0;
	}

	std::strong_ordering operator<=>(const Path& lhs, const Path& rhs) noexcept
	{
		return lhs.Compare(rhs) <=> 0;
	}

	Path& Path::operator/=(const Path& other)
	{
		m_path += PlatformDirectoryDivider + other.m_path;
		return *this;
	}

	Path::operator PathView() const noexcept
	{
		return PathView(m_path);
	}

	Path& Path::operator+=(const Path& other)
	{
		m_path += other.m_path;
		return *this;
	}
	
	Path& Path::operator+=(const WStringView other)
	{
		m_path += WString(other);
		return *this;
	}

	Path operator+(const Path& lhs, const WString::value_type* rhs) noexcept
	{
		return Path(lhs.m_path + rhs);
	}

	Path operator+(const Path& lhs, const String::value_type* rhs) noexcept
	{
		return Path(lhs.m_path + WString(WString::CtorConvert(), rhs, strlen(rhs)));
	}

	Path operator+(const Path& lhs, const Path& rhs) noexcept
	{
		return Path(lhs.m_path + rhs.m_path);
	}

	Path operator+(const Path& lhs, const WStringView rhs) noexcept
	{
		return Path(lhs.m_path + WString(rhs));
	}

	Path operator+(const Path& lhs, const StringView rhs) noexcept
	{
		return Path(lhs.m_path + WString(WString::CtorConvert(), rhs.data(), rhs.size()));
	}

	void Path::Clear() noexcept
	{
		m_path.clear();
	}
	
	void Path::Swap(Path& other)
	{
		std::swap(m_path, other.m_path);
	}
	
	int32_t Path::Compare(const WStringView other) const
	{
		std::filesystem::path tempThis(m_path.begin(), m_path.end());
		std::wstring_view tempOther(other.begin(), other.size());

		return tempThis.compare(tempOther);
	}

	int32_t Path::Compare(const Filesystem::Path& other) const
	{
		return Compare(WStringView(other.ToWString()));
	}

	Path& Path::MakePreferred()
	{
		std::replace(m_path.begin(), m_path.end(), NonPlatformDirectoryDivider, PlatformDirectoryDivider);
		return *this;
	}

	Path& Path::RemoveFilename()
	{
		const Helpers::PathParts pathParts = Helpers::ParsePath(m_path);
		m_path.erase(static_cast<size_t>(pathParts.filename.data() - m_path.begin()));

		return *this;
	}
	
	Path& Path::ReplaceFilename(const Path& replacement)
	{
		RemoveFilename();
		return operator/=(replacement);
	}
	
	Path& Path::ReplaceExtension(const Path& replacement)
	{
		const Helpers::PathParts pathParts = Helpers::ParsePath(m_path);
		m_path.erase(static_cast<size_t>(pathParts.extension.data() - m_path.begin()));

		return *this;
	}

	String Path::ToString() const
	{
		String temp;
		temp.assign_convert(m_path.begin(), m_path.size());

		return temp;
	}
	
	const WString& Path::ToWString() const
	{
		return m_path;
	}

	const wchar_t* Path::CStr() const
	{
		return m_path.c_str();
	}

	Path Path::RootName() const
	{
		return ParseRootName();
	}
	
	Path Path::RootDirectory() const
	{
		return ParseRootDirectory();
	}
	
	Path Path::RootPath() const
	{
		return ParseRootPath();
	}
	
	Path Path::RelativePath() const
	{
		return ParseRelativePath();
	}
	
	Path Path::ParentPath() const
	{
		return ParseParentPath();
	}
	
	Path Path::Filename() const
	{
		return ParseFilename();
	}
	
	Path Path::Stem() const
	{
		return ParseStem();
	}
	
	Path Path::Extension() const
	{
		return ParseExtension();
	}
	
	Path Path::LexicallyNormal() const
	{
		const Helpers::RootParts rootParts = Helpers::ParseRoot(m_path);

		WString result;

		result.append(rootParts.rootName.data(), rootParts.rootName.size());
		result.append(rootParts.rootDirectory.data(), rootParts.rootDirectory.size());

		Vector<WStringView> stack;

		WStringView relative = ParseRelativePath();

		const wchar_t* p = relative.data();
		const wchar_t* end = p + relative.size();

		while (p < end)
		{
			const wchar_t* comp = p;

			while (p < end && *p != L'\\' && *p != L'/')
			{
				++p;
			}

			WStringView part(comp, p - comp);

			if (part == L"." || part.size() == 0)
			{
			}
			else if (part == L"..")
			{
				if (!stack.empty())
				{
					stack.pop_back();
				}
			}
			else
			{
				stack.push_back(part);
			}

			if (p < end)
			{
				++p;
			}
		}

		for (size_t i = 0; i < stack.size(); i++)
		{
			if (!result.empty() && result.back() != L'\\')
			{
				result.push_back(L'\\');
			}

			result.append(stack[i].data(), stack[i].size());
		}

		return result;
	}

	PathView Path::RootNameView() const
	{
		return ParseRootName();
	}

	PathView Path::RootDirectoryView() const
	{
		return ParseRootDirectory();
	}

	PathView Path::RootPathView() const
	{
		return ParseRootPath();
	}

	PathView Path::RelativePathView() const
	{
		return ParseRelativePath();
	}

	PathView Path::ParentPathView() const
	{
		return ParseParentPath();
	}

	PathView Path::FilenameView() const
	{
		return ParseFilename();
	}

	PathView Path::StemView() const
	{
		return ParseStem();
	}

	PathView Path::ExtensionView() const
	{
		return ParseExtension();
	}

	bool Path::IsEmpty() const
	{
		return m_path.empty();
	}
	
	bool Path::HasRootPath() const
	{
		return !ParseRootPath().empty();
	}
	
	bool Path::HasRootName() const
	{
		return !ParseRootName().empty();
	}
	
	bool Path::HasRootDirectory() const
	{
		return !ParseRootDirectory().empty();
	}
	
	bool Path::HasRelativePath() const
	{
		return !ParseRelativePath().empty();
	}
	
	bool Path::HasParentPath() const
	{
		return !ParseParentPath().empty();
	}
	
	bool Path::HasFilename() const
	{
		return !ParseFilename().empty();
	}
	
	bool Path::HasStem() const
	{
		return !ParseStem().empty();
	}
	
	bool Path::HasExtension() const
	{
		return !ParseExtension().empty();
	}
	
	bool Path::IsAbsolute() const
	{
		const Helpers::RootParts rootParts = Helpers::ParseRoot(m_path);

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
	
	bool Path::IsRelative() const
	{
		return !IsAbsolute();
	}

	WStringView Path::ParseRootName() const
	{
		const Helpers::RootParts rootParts = Helpers::ParseRoot(m_path);
		return rootParts.rootName;
	}

	WStringView Path::ParseRootDirectory() const
	{
		const Helpers::RootParts rootParts = Helpers::ParseRoot(m_path);
		return rootParts.rootDirectory;
	}

	WStringView Path::ParseRootPath() const
	{
		const Helpers::RootParts rootParts = Helpers::ParseRoot(m_path);
		return WStringView(rootParts.rootName.begin(), rootParts.rootName.size() + rootParts.rootDirectory.size());
	}

	WStringView Path::ParseRelativePath() const
	{
		const Helpers::RootParts rootParts = Helpers::ParseRoot(m_path);

		const wchar_t* begin = m_path.data() + rootParts.pathStart;
		size_t length = m_path.size() - rootParts.pathStart;

		return WStringView(begin, length);
	}

	WStringView Path::ParseParentPath() const
	{
		const Helpers::PathParts pathParts = Helpers::ParsePath(m_path);
		return pathParts.parent;
	}

	WStringView Path::ParseFilename() const
	{
		const Helpers::PathParts pathParts = Helpers::ParsePath(m_path);
		return pathParts.filename;
	}

	WStringView Path::ParseStem() const
	{
		const Helpers::PathParts pathParts = Helpers::ParsePath(m_path);
		return pathParts.stem;
	}

	WStringView Path::ParseExtension() const
	{
		const Helpers::PathParts pathParts = Helpers::ParsePath(m_path);
		return pathParts.extension;
	}

	Path operator/(const Path& lhs, const Path& rhs)
	{
		return lhs.m_path + Path::PlatformDirectoryDivider + rhs.m_path;
	}
}
