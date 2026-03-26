#pragma once

#include "CoreUtilities/Config.h"
#include "CoreUtilities/CompilerTraits.h"
#include "CoreUtilities/String/StringView.h"

namespace Filesystem
{
	class PathView
	{
	public:
		PathView() noexcept = default;
		PathView(const PathView& other) noexcept = default;
		PathView& operator=(const PathView& other) noexcept = default;

		VTCOREUTIL_API friend bool operator==(const PathView& lhs, const PathView& rhs) noexcept;
		VTCOREUTIL_API friend bool operator==(const PathView& lhs, const WStringView& rhs) noexcept;
		VTCOREUTIL_API friend bool operator==(const PathView& lhs, const wchar_t* rhs) noexcept;
		VTCOREUTIL_API friend std::strong_ordering operator<=>(const PathView& lhs, const PathView& rhs) noexcept;

		VT_INLINE WStringView::const_iterator begin() const noexcept { return m_stringView.begin(); }
		VT_INLINE WStringView::const_iterator cbegin() const noexcept { return m_stringView.cbegin(); }
		VT_INLINE WStringView::const_iterator end() const noexcept { return m_stringView.end(); }
		VT_INLINE WStringView::const_iterator cend() const noexcept { return m_stringView.cend(); }

		VT_INLINE WStringView::const_reverse_iterator rbegin() const noexcept { return m_stringView.rbegin(); }
		VT_INLINE WStringView::const_reverse_iterator crbegin() const noexcept { return m_stringView.crbegin(); }
		VT_INLINE WStringView::const_reverse_iterator rend() const noexcept { return m_stringView.rend(); }
		VT_INLINE WStringView::const_reverse_iterator crend() const noexcept { return m_stringView.crend(); }

		VTCOREUTIL_API const wchar_t* CStr() const;

		VTCOREUTIL_API int32_t Compare(const WStringView other) const;
		VTCOREUTIL_API int32_t Compare(const PathView& other) const;

		VTCOREUTIL_API PathView RootName() const;
		VTCOREUTIL_API PathView RootDirectory() const;
		VTCOREUTIL_API PathView RootPath() const;
		VTCOREUTIL_API PathView RelativePath() const;
		VTCOREUTIL_API PathView ParentPath() const;
		VTCOREUTIL_API PathView Filename() const;
		VTCOREUTIL_API PathView Stem() const;
		VTCOREUTIL_API PathView Extension() const;

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

	private:
		friend class Path;

		PathView(const WStringView pathView);

		WStringView m_stringView;
	};
}
