#pragma once

#include "CoreUtilities/Config.h"
#include "CoreUtilities/String/VoltString.h"

#include "CoreUtilities/Filesystem/PathView.h"

#include <format>

namespace Filesystem
{
	class Path
	{
	public:
		using StringType = WString;

		VTCOREUTIL_API Path();
		VTCOREUTIL_API ~Path();

		VTCOREUTIL_API Path(WStringView string);
		VTCOREUTIL_API Path(const WString& string);

		VTCOREUTIL_API Path(StringView string);
		VTCOREUTIL_API Path(const String& string);

		VTCOREUTIL_API Path(const WString::value_type* value);
		VTCOREUTIL_API Path(const String::value_type* value);

		VTCOREUTIL_API Path(const Path& other);
		VTCOREUTIL_API Path& operator=(const Path& other);

		VTCOREUTIL_API Path(Path&& other) noexcept;
		VTCOREUTIL_API Path& operator=(Path&& other) noexcept;

		VTCOREUTIL_API Path& operator/=(const Path& other);

		VTCOREUTIL_API friend bool operator==(const Path& lhs, const Path& rhs) noexcept;
		VTCOREUTIL_API friend std::strong_ordering operator<=>(const Path& lhs, const Path& rhs) noexcept;

		VTCOREUTIL_API friend Path operator+(const Path& lhs, const Path& rhs) noexcept;
		VTCOREUTIL_API friend Path operator+(const Path& lhs, const WStringView rhs) noexcept;
		VTCOREUTIL_API friend Path operator+(const Path& lhs, const StringView rhs) noexcept;
		VTCOREUTIL_API friend Path operator+(const Path& lhs, const WString::value_type* rhs) noexcept;
		VTCOREUTIL_API friend Path operator+(const Path& lhs, const String::value_type* rhs) noexcept;

		VTCOREUTIL_API operator PathView() const noexcept;

		template<typename T>
		Path& operator/=(const T& other);

		VTCOREUTIL_API friend Path operator/(const Path& lhs, const Path& rhs);

		VTCOREUTIL_API Path& operator+=(const Path& other);
		VTCOREUTIL_API Path& operator+=(const WStringView other);

		/*
			Clears the path.
		*/
		VTCOREUTIL_API void Clear() noexcept;
		
		/*
			Swaps this path with another.
		*/
		VTCOREUTIL_API void Swap(Path& other);

		/*
			Lexicographical compare of two paths
		*/
		VTCOREUTIL_API int32_t Compare(const WStringView other) const;
		VTCOREUTIL_API int32_t Compare(const Filesystem::Path& other) const;

		/*
			Converts all directory seperators with OS preferred seperators.
		*/
		VTCOREUTIL_API Path& MakePreferred();

		/*
			Removes the filename component.
		*/
		VTCOREUTIL_API Path& RemoveFilename();

		/*
			Replaces the filename component.
		*/
		VTCOREUTIL_API Path& ReplaceFilename(const Path& replacement);

		/*
			Relaces the extension
		*/
		VTCOREUTIL_API Path& ReplaceExtension(const Path& replacement);

		VTCOREUTIL_API String ToString() const;
		VTCOREUTIL_API const WString& ToWString() const;

		VTCOREUTIL_API const wchar_t* CStr() const;

		VTCOREUTIL_API Path RootName() const;
		VTCOREUTIL_API Path RootDirectory() const;
		VTCOREUTIL_API Path RootPath() const;
		VTCOREUTIL_API Path RelativePath() const;
		VTCOREUTIL_API Path ParentPath() const;
		VTCOREUTIL_API Path Filename() const;
		VTCOREUTIL_API Path Stem() const;
		VTCOREUTIL_API Path Extension() const;
		VTCOREUTIL_API Path LexicallyNormal() const;

		VTCOREUTIL_API PathView RootNameView() const;
		VTCOREUTIL_API PathView RootDirectoryView() const;
		VTCOREUTIL_API PathView RootPathView() const;
		VTCOREUTIL_API PathView RelativePathView() const;
		VTCOREUTIL_API PathView ParentPathView() const;
		VTCOREUTIL_API PathView FilenameView() const;
		VTCOREUTIL_API PathView StemView() const;
		VTCOREUTIL_API PathView ExtensionView() const;

		VTCOREUTIL_API bool IsEmpty() const;
		VTCOREUTIL_API bool HasRootPath() const;
		VTCOREUTIL_API bool HasRootName() const;
		VTCOREUTIL_API bool HasRootDirectory() const;
		VTCOREUTIL_API bool HasRelativePath() const;
		VTCOREUTIL_API bool HasParentPath() const;
		VTCOREUTIL_API bool HasFilename() const;
		VTCOREUTIL_API bool HasStem() const;
		VTCOREUTIL_API bool HasExtension() const;

		VTCOREUTIL_API bool IsAbsolute() const;
		VTCOREUTIL_API bool IsRelative() const;

		inline static constexpr wchar_t PlatformDirectoryDivider = '\\';
		inline static constexpr wchar_t NonPlatformDirectoryDivider = '/';

		const StringType::value_type* begin() const { return m_path.begin(); }
		const StringType::value_type* end() const { return m_path.end(); }

	private:
		WStringView ParseRootName() const;
		WStringView ParseRootDirectory() const;
		WStringView ParseRootPath() const;
		WStringView ParseRelativePath() const;
		WStringView ParseParentPath() const;
		WStringView ParseFilename() const;
		WStringView ParseStem() const;
		WStringView ParseExtension() const;

		StringType m_path;
	};

	template<typename T>
	inline Path& Path::operator/=(const T& other)
	{
		return operator/=(Path(other));
	}
}

namespace std
{
	template<typename CharT>
	struct formatter<Filesystem::Path, CharT>
	{
		formatter<basic_string_view<CharT>, CharT> underlying;

		constexpr auto parse(basic_format_parse_context<CharT>& ctx)
		{
			return underlying.parse(ctx);
		}

		template<typename FormatContext>
		auto format(const Filesystem::Path& path, FormatContext& ctx) const
		{
			if constexpr (std::is_same_v<CharT, wchar_t>)
			{
				const WString& str = path.ToWString();

				return underlying.format(
					std::basic_string_view<wchar_t>(str.c_str(), str.size()),
					ctx
				);
			}
			else
			{
				const String str = path.ToString();

				return underlying.format(
					std::basic_string_view<char>(str.c_str(), str.size()),
					ctx
				);
			}
		}
	};

	template<>
	struct hash<Filesystem::Path>
	{
		size_t operator()(const Filesystem::Path& x) const
		{
			return std::hash<WString>()(x.ToWString());
		}
	};
}
