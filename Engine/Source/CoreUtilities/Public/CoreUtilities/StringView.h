#pragma once

#include "StringAlgorithm.h"

#include <intsafe.h>
#include <limits.h>
#include <algorithm>

// License in Engine\Source\ThirdParty\eastl

template<typename T>
class BasicStringView
{
public:
	typedef BasicStringView<T> this_type;
	typedef T value_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T* iterator;
	typedef const T* const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	typedef size_t size_type;
	typedef std::ptrdiff_t difference_type;

	static const constexpr size_type npos = size_type(-1);

	constexpr BasicStringView() noexcept : m_begin(nullptr), m_count(0) {}
	constexpr BasicStringView(const BasicStringView& other) noexcept = default;
	constexpr BasicStringView(const T* s, size_type count) : m_begin(s), m_count(count) {}
	constexpr BasicStringView(const T* s) : m_begin(s), m_count(s != nullptr ? StringAlgorithm::Strlen(s) : 0) {}
	BasicStringView& operator=(const BasicStringView& view) = default;

	constexpr const_iterator begin() const noexcept { return m_begin; }
	constexpr const_iterator cbegin() const noexcept { return m_begin; }
	constexpr const_iterator end() const noexcept { return m_begin + m_count; }
	constexpr const_iterator cend() const noexcept { return m_begin + m_count; }
	constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(m_begin + m_count); }
	constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(m_begin + m_count); }
	constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(m_begin); }
	constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator(m_begin); }

	constexpr const_pointer data() const { return m_begin; }
	constexpr const_reference front() const
	{
		VT_ASSERT_MSG(!empty(), "Calling front is undefined if StringView is empty!");
		return m_begin[0];
	}

	constexpr const_reference back() const
	{
		VT_ASSERT_MSG(!empty(), "Calling back is undefined if StringView is empty!");
		return m_begin[m_count - 1];
	}

	constexpr const_reference operator[](size_type pos) const
	{
		return m_begin[pos];
	}

	constexpr const_reference at(size_type pos) const
	{
		VT_ASSERT_MSG(pos < m_count, "StringView::at - Out of range!");
		return m_begin[pos];
	}

	constexpr size_type size() const noexcept { return m_count; }
	constexpr size_type length() const noexcept { return m_count; }

	constexpr size_type max_size() const noexcept { return std::numeric_limits<size_type>::max(); }
	constexpr bool empty() const noexcept { return m_count == 0; }

	constexpr void swap(BasicStringView& other)
	{
		std::swap(m_begin, other.m_begin);
		std::swap(m_count, other.m_count);
	}

	constexpr void remove_prefix(size_type n)
	{
		VT_ASSERT_MSG(n > m_count, "Undefined if moving past end of string!");
		m_begin += n;
		m_count -= n;
	}

	constexpr void remove_suffix(size_type n)
	{
		VT_ASSERT_MSG(n > m_count, "Undefined if moving past end of string!");
		m_count -= n;
	}

	size_type copy(T* pDestination, size_type count, size_type pos = 0) const
	{
		VT_ASSERT_MSG(pos <= m_count, "StringView::copy - Out of range!");

		count = std::min(count, m_count - pos);
		auto* pResult = memmove(pDestination, m_begin + pos, (size_t)((m_begin + pos + count) - m_begin));
		return pResult - pDestination;
	}

	constexpr BasicStringView substr(size_type pos = 0, size_type count = npos) const
	{
		VT_ASSERT_MSG(pos <= m_count, "StringView::substr - Out of range!");

		count = std::min(count, m_count - pos);
		return this_type(m_begin + pos, count);
	}

	static constexpr int compare(const T* pBegin1, const T* pEnd1, const T* pBegin2, const T* pEnd2)
	{
		const ptrdiff_t n1 = pEnd1 - pBegin1;
		const ptrdiff_t n2 = pEnd2 - pBegin2;
		const ptrdiff_t nMin = std::min(n1, n2);
		const int cmp = __builtin_memcmp(pBegin1, pBegin2, (size_type)nMin);

		return (cmp != 0 ? cmp : (n1 < n2 ? -1 : (n1 > n2 ? 1 : 0)));
	}

	constexpr int compare(BasicStringView sw) const noexcept
	{
		return compare(m_begin, m_begin + m_count, sw.m_begin, sw.m_begin + sw.m_count);
	}

	constexpr int compare(size_type pos1, size_type count1, BasicStringView sw) const
	{
		return substr(pos1, count1).compare(sw);
	}

	constexpr int compare(size_type pos1, size_type count1, BasicStringView sw, size_type pos2, size_type count2) const
	{
		return substr(pos1, count1).compare(sw.substr(pos2, count2));
	}

	constexpr int compare(const T* s) const { return compare(BasicStringView(s)); }

	constexpr int compare(size_type pos1, size_type count1, const T* s) const
	{
		return substr(pos1, count1).compare(BasicStringView(s));
	}

	constexpr int compare(size_type pos1, size_type count1, const T* s, size_type count2) const
	{
		return substr(pos1, count1).compare(BasicStringView(s, count2));
	}

	constexpr size_type find(BasicStringView sw, size_type pos = 0) const noexcept
	{
		auto* pEnd = m_begin + m_count;
		if ((((npos - sw.size()) >= pos) && (pos + sw.size()) <= m_count))
		{
			const value_type* const pTemp = std::search(m_begin + pos, pEnd, sw.data(), sw.data() + sw.size());

			if ((pTemp != pEnd) || (sw.size() == 0))
			{
				return (size_type)(pTemp - m_begin);
			}
		}
		return npos;
	}

	constexpr size_type find(T c, size_type pos = 0) const noexcept
	{
		return find(BasicStringView(&c, 1), pos);
	}

	constexpr size_type find(const T* s, size_type pos, size_type count) const
	{
		return find(BasicStringView(s, count), pos);
	}

	constexpr size_type find(const T* s, size_type pos = 0) const { return find(BasicStringView(s), pos); }

	constexpr size_type rfind(BasicStringView sw, size_type pos = npos) const noexcept
	{
		return rfind(sw.m_begin, pos, sw.m_count);
	}

	constexpr size_type rfind(T c, size_type pos = npos) const noexcept
	{
		if (m_count > 0)
		{
			const value_type* const pEnd = m_begin + std::min(m_count - 1, pos) + 1;
			const value_type* const pResult = StringAlgorithm::StringRFind(pEnd, m_begin, c);

			if (pResult != m_begin)
				return (size_type)((pResult - 1) - m_begin);
		}
		return npos;
	}

	constexpr size_type rfind(const T* s, size_type pos, size_type n) const
	{
		if (n <= m_count)
		{
			if (n > 0)
			{
				const const_iterator pEnd = m_begin + std::min(m_count - n, pos) + n;
				const const_iterator pResult = StringAlgorithm::StringRFind(m_begin, pEnd, s, s + n);

				if (pResult != pEnd)
					return (size_type)(pResult - m_begin);
			}
			else
			{
				return std::min(m_count, pos);
			}
		}
		return npos;
	}

	constexpr size_type rfind(const T* s, size_type pos = npos) const
	{
		return rfind(s, pos, (size_type)StringAlgorithm::Strlen(s));
	}

	constexpr size_type find_first_of(BasicStringView sw, size_type pos = 0) const noexcept
	{
		return find_first_of(sw.m_begin, pos, sw.m_count);
	}

	constexpr size_type find_first_of(T c, size_type pos = 0) const noexcept { return find(c, pos); }

	constexpr size_type find_first_of(const T* s, size_type pos, size_type n) const
	{
		// If position is >= size, we return npos.
		if (pos < m_count)
		{
			const value_type* const pBegin = m_begin + pos;
			const value_type* const pEnd = m_begin + m_count;
			const const_iterator pResult = StringAlgorithm::StringFindFirstOf(pBegin, pEnd, s, s + n);

			if (pResult != pEnd)
			{
				return (size_type)(pResult - m_begin);
			}
		}
		return npos;
	}

	constexpr size_type find_first_of(const T* s, size_type pos = 0) const
	{
		return find_first_of(s, pos, (size_type)StringAlgorithm::Strlen(s));
	}

	constexpr size_type find_last_of(BasicStringView sw, size_type pos = npos) const noexcept
	{
		return find_last_of(sw.m_begin, pos, sw.m_count);
	}

	constexpr size_type find_last_of(T c, size_type pos = npos) const noexcept { return rfind(c, pos); }

	constexpr size_type find_last_of(const T* s, size_type pos, size_type n) const
	{
		// If n is zero or position is >= size, we return npos.
		if (m_count > 0)
		{
			const value_type* const pEnd = m_begin + std::min(m_count - 1, pos) + 1;
			const value_type* const pResult = StringAlgorithm::StringRFindFirstOf(pEnd, m_begin, s, s + n);

			if (pResult != m_begin)
				return (size_type)((pResult - 1) - m_begin);
		}
		return npos;
	}

	constexpr size_type find_last_of(const T* s, size_type pos = npos) const
	{
		return find_last_of(s, pos, (size_type)StringAlgorithm::Strlen(s));
	}

	constexpr size_type find_first_not_of(BasicStringView sw, size_type pos = 0) const noexcept
	{
		return find_first_not_of(sw.m_begin, pos, sw.m_count);
	}

	constexpr size_type find_first_not_of(T c, size_type pos = 0) const noexcept
	{
		if (pos <= m_count)
		{
			const auto pEnd = m_begin + m_count;
			const const_iterator pResult = StringAlgorithm::StringFindFirstNotOf(m_begin + pos, pEnd, &c, &c + 1);

			if (pResult != pEnd)
			{
				return (size_type)(pResult - m_begin);
			}
		}
		return npos;
	}

	constexpr size_type find_first_not_of(const T* s, size_type pos, size_type n) const
	{
		if (pos <= m_count)
		{
			const auto pEnd = m_begin + m_count;
			const const_iterator pResult = StringAlgorithm::StringFindFirstNotOf(m_begin + pos, pEnd, s, s + n);

			if (pResult != pEnd)
			{
				return (size_type)(pResult - m_begin);
			}
		}
		return npos;
	}

	constexpr size_type find_first_not_of(const T* s, size_type pos = 0) const
	{
		return find_first_not_of(s, pos, (size_type)StringAlgorithm::Strlen(s));
	}

	constexpr size_type find_last_not_of(BasicStringView sw, size_type pos = npos) const noexcept
	{
		return find_last_not_of(sw.m_begin, pos, sw.m_count);
	}

	constexpr size_type find_last_not_of(T c, size_type pos = npos) const noexcept
	{
		if (m_count > 0)
		{
			// Todo: Possibly make a specialized version of CharTypeStringRFindFirstNotOf(pBegin, pEnd, c).
			const value_type* const pEnd = m_begin + std::min(m_count - 1, pos) + 1;
			const value_type* const pResult = StringAlgorithm::StringRFindFirstNotOf(pEnd, m_begin, &c, &c + 1);

			if (pResult != m_begin)
			{
				return (size_type)((pResult - 1) - m_begin);
			}
		}
		return npos;
	}

	constexpr size_type find_last_not_of(const T* s, size_type pos, size_type n) const
	{
		if (m_count > 0)
		{
			const value_type* const pEnd = m_begin + std::min(m_count - 1, pos) + 1;
			const value_type* const pResult = StringAlgorithm::StringRFindFirstNotOf(pEnd, m_begin, s, s + n);

			if (pResult != m_begin)
			{
				return (size_type)((pResult - 1) - m_begin);
			}
		}
		return npos;
	}

	constexpr size_type find_last_not_of(const T* s, size_type pos = npos) const
	{
		return find_last_not_of(s, pos, (size_type)StringAlgorithm::Strlen(s));
	}

	constexpr bool starts_with(BasicStringView x) const noexcept
	{
		return (size() >= x.size()) && (compare(0, x.size(), x) == 0);
	}

	constexpr bool starts_with(T x) const noexcept
	{
		return starts_with(BasicStringView(&x, 1));
	}

	constexpr bool starts_with(const T* s) const
	{
		return starts_with(BasicStringView(s));
	}

	// ends_with
	constexpr bool ends_with(BasicStringView x) const noexcept
	{
		return (size() >= x.size()) && (compare(size() - x.size(), npos, x) == 0);
	}

	constexpr bool ends_with(T x) const noexcept
	{
		return ends_with(BasicStringView(&x, 1));
	}

	constexpr bool ends_with(const T* s) const
	{
		return ends_with(BasicStringView(s));
	}

protected:
	const_pointer m_begin = nullptr;
	size_type m_count = 0;
};

// Extra template parameter is to get around a known limitation in MSVC's ABI (name decoration)
template <class CharT, int = 0>
inline constexpr bool operator==(BasicStringView<CharT> lhs, BasicStringView<CharT> rhs) noexcept
{
	return (lhs.size() == rhs.size()) && (lhs.compare(rhs) == 0);
}

// type_identity_t is used in this context to forcefully trigger conversion operators towards BasicStringView.
// Mostly we want basic_string::operator BasicStringView() to kick-in to be able to compare strings and string_views.
template <class CharT, int = 1>
inline constexpr bool operator==(std::type_identity_t<BasicStringView<CharT>> lhs, BasicStringView<CharT> rhs) noexcept
{
	return (lhs.size() == rhs.size()) && (lhs.compare(rhs) == 0);
}

template <class CharT, int = 2>
inline constexpr bool operator==(BasicStringView<CharT> lhs, std::type_identity_t<BasicStringView<CharT>> rhs) noexcept
{
	return (lhs.size() == rhs.size()) && (lhs.compare(rhs) == 0);
}

template <class CharT>
inline constexpr auto operator<=>(BasicStringView<CharT> lhs, BasicStringView<CharT> rhs) noexcept
{
	return static_cast<std::weak_ordering>(lhs.compare(rhs) <=> 0);
}

template <class CharT>
inline constexpr auto operator<=>(BasicStringView<CharT> lhs, typename BasicStringView<CharT>::const_pointer rhs) noexcept
{
	typedef BasicStringView<CharT> view_type;
	return static_cast<std::weak_ordering>(lhs <=> static_cast<view_type>(rhs));
}

typedef BasicStringView<char> StringView;
typedef BasicStringView<wchar_t> WStringView;

typedef BasicStringView<char8_t> U8StringView;
typedef BasicStringView<char16_t> U16StringView;
typedef BasicStringView<char32_t> U32StringView;

namespace std
{
	template<typename T> struct hash;

	template<> struct hash<StringView>
	{
		size_t operator()(const StringView& x) const
		{
			StringView::const_iterator p = x.cbegin();
			StringView::const_iterator end = x.cend();

			uint32_t result = 2166136261u;
			while (p != end)
			{
				result = (result * 16777619) ^ (uint8_t)*p++;
			}

			return static_cast<size_t>(result);
		}
	};

	template<> struct hash<U8StringView>
	{
		size_t operator()(const U8StringView& x) const
		{
			U8StringView::const_iterator p = x.cbegin();
			U8StringView::const_iterator end = x.cend();

			uint32_t result = 2166136261u;
			while (p != end)
			{
				result = (result * 16777619) ^ (uint8_t)*p++;
			}

			return static_cast<size_t>(result);
		}
	};

	template<> struct hash<U16StringView>
	{
		size_t operator()(const U16StringView& x) const
		{
			U16StringView::const_iterator p = x.cbegin();
			U16StringView::const_iterator end = x.cend();

			uint32_t result = 2166136261u;
			while (p != end)
			{
				result = (result * 16777619) ^ (uint8_t)*p++;
			}

			return static_cast<size_t>(result);
		}
	};

	template<> struct hash<U32StringView>
	{
		size_t operator()(const U32StringView& x) const
		{
			U32StringView::const_iterator p = x.cbegin();
			U32StringView::const_iterator end = x.cend();

			uint32_t result = 2166136261u;
			while (p != end)
			{
				result = (result * 16777619) ^ (uint8_t)*p++;
			}

			return static_cast<size_t>(result);
		}
	};

	template<> struct hash<WStringView>
	{
		size_t operator()(const WStringView& x) const
		{
			WStringView::const_iterator p = x.cbegin();
			WStringView::const_iterator end = x.cend();

			uint32_t result = 2166136261u;
			while (p != end)
			{
				result = (result * 16777619) ^ (uint8_t)*p++;
			}

			return static_cast<size_t>(result);
		}
	};
}
