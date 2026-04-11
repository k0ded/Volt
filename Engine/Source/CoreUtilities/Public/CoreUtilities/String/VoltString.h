#pragma once

#include "CoreUtilities/String/StringView.h"
#include "CoreUtilities/String/CharConversion.h"
#include "CoreUtilities/CompressedPair.h"
#include "CoreUtilities/Allocators/ContainerAllocators.h"
#include "CoreUtilities/VoltAssert.h"

#include <algorithm>
#include <iterator>
#include <locale>
#include <format>

// License in Engine\Source\ThirdParty\eastl

template<typename T, typename Allocator = DefaultHeapAllocator>
class BasicString
{
public:
	typedef BasicString<T, Allocator> this_type;
	typedef BasicStringView<T> view_type;
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
	typedef Allocator::template ForElementType<T> allocator_type;

	static const constexpr size_type npos = size_type(-1);

public:
	// CtorDoNotInitialize exists so that we can create a constructor that allocates but doesn't
	// initialize and also doesn't collide with any other constructor declaration.
	struct CtorDoNotInitialize {};

	// CtorConvert exists so that we can have a constructor that implements string encoding
	// conversion, such as between UCS2 char16_t and UTF8 char8_t.
	struct CtorConvert {};

protected:
	// Masks used to determine if we are in SSO or Heap
#ifdef BIG_ENDIAN_NOT_IMPLEMENTED
	// Big Endian use LSB, unless we want to reorder struct layouts on endianness, Bit is set when we are in Heap
	static constexpr size_type kHeapMask = 0x1;
	static constexpr size_type kSSOMask = 0x1;
#else
	// Little Endian use MSB
	static constexpr size_type kHeapMask = ~(size_type(~size_type(0)) >> 1);
	static constexpr size_type kSSOMask = 0x80;
#endif

public:
#ifdef BIG_ENDIAN_NOT_IMPLEMENTED
	static constexpr size_type kMaxSize = (~kHeapMask) >> 1;
#else
	static constexpr size_type kMaxSize = ~kHeapMask;
#endif

protected:
	// The view of memory when the string data is obtained from the allocator.
	struct HeapLayout
	{
		value_type* m_begin;  // Begin of string.
		size_type m_size;     // Size of the string. Number of characters currently in the string, not including the trailing '0'
		size_type m_capacity; // Capacity of the string. Number of characters string can hold, not including the trailing '0'
	};

	template <typename CharT, size_t = sizeof(CharT)>
	struct SSOPadding
	{
		char padding[sizeof(CharT) - sizeof(char)];
	};

	template <typename CharT>
	struct SSOPadding<CharT, 1>
	{
		// template specialization to remove the padding structure to avoid warnings on zero length arrays
		// also, this allows us to take advantage of the empty-base-class optimization.
	};

	// The view of memory when the string data is able to store the string data locally (without a heap allocation).
	struct SSOLayout
	{
		static constexpr size_type SSO_CAPACITY = (sizeof(HeapLayout) - sizeof(char)) / sizeof(value_type);

		// mnSize must correspond to the last byte of HeapLayout.mnCapacity, so we don't want the compiler to insert
		// padding after mnSize if sizeof(value_type) != 1; Also ensures both layouts are the same size.
		struct SSOSize : SSOPadding<value_type>
		{
			char m_remainingSize;
		};

		value_type m_data[SSO_CAPACITY]; // Local buffer for string data.
		SSOSize m_remainingSizeField;
	};

	// This view of memory is a utility structure for easy copying of the string data.
	struct RawLayout
	{
		char m_buffer[sizeof(HeapLayout)];
	};

	static_assert(sizeof(SSOLayout) == sizeof(HeapLayout), "heap and sso layout structures must be the same size");
	static_assert(sizeof(HeapLayout) == sizeof(RawLayout), "heap and raw layout structures must be the same size");

	// This implements the 'short string optimization' or SSO. SSO reuses the existing storage of string class to
	// hold string data short enough to fit therefore avoiding a heap allocation. The number of characters stored in
	// the string SSO buffer is variable and depends on the string character width. This implementation favors a
	// consistent string size than increasing the size of the string local data to accommodate a consistent number
	// of characters despite character width.
	struct Layout
	{
		union
		{
			HeapLayout heap;
			SSOLayout sso;
			RawLayout raw;
		};

		Layout() { ResetToSSO(); } // start as SSO by default
		Layout(const Layout& other) { Copy(*this, other); }
		Layout(Layout&& other) { Move(*this, other); }
		Layout& operator=(const Layout& other) { Copy(*this, other); return *this; }
		Layout& operator=(Layout&& other) { Move(*this, other); return *this; }

		// We are using Heap when the bit is set, easier to conceptualize checking IsHeap instead of IsSSO
		inline bool IsHeap() const noexcept { return !!(sso.m_remainingSizeField.m_remainingSize & kSSOMask); }
		inline bool IsSSO() const noexcept { return !IsHeap(); }
		inline value_type* SSOBufferPtr() noexcept { return sso.m_data; }
		inline const value_type* SSOBufferPtr() const noexcept { return sso.m_data; }

		// Largest value for SSO.mnSize == 23, which has two LSB bits set, but on big-endian (BE)
		// use least significant bit (LSB) to denote heap so shift.
		inline size_type GetSSOSize() const noexcept
		{
#ifdef BIG_ENDIAN_NOT_IMPLEMENTED
			return SSOLayout::SSO_CAPACITY - (sso.m_remainingSizeField.m_remainingSize >> 2);
#else
			return (SSOLayout::SSO_CAPACITY - sso.m_remainingSizeField.m_remainingSize);
#endif
		}
		inline size_type GetHeapSize() const noexcept { return heap.m_size; }
		inline size_type GetSize() const noexcept { return IsHeap() ? GetHeapSize() : GetSSOSize(); }

		inline void SetSSOSize(size_type size) noexcept
		{
#ifdef BIG_ENDIAN_NOT_IMPLEMENTED
			sso.m_remainingSizeField.m_remainingSize = (char)((SSOLayout::SSO_CAPACITY - size) << 2);
#else
			sso.m_remainingSizeField.m_remainingSize = (char)(SSOLayout::SSO_CAPACITY - size);
#endif
		}

		inline void SetHeapSize(size_type size) noexcept { heap.m_size = size; }
		inline void SetSize(size_type size) noexcept { IsHeap() ? SetHeapSize(size) : SetSSOSize(size); }

		inline size_type GetRemainingCapacity() const noexcept { return size_type(CapacityPtr() - EndPtr()); }

		inline value_type* HeapBeginPtr() noexcept { return heap.m_begin; };
		inline const value_type* HeapBeginPtr() const noexcept { return heap.m_begin; };

		inline value_type* SSOBeginPtr() noexcept { return sso.m_data; }
		inline const value_type* SSOBeginPtr() const noexcept { return sso.m_data; }

		inline value_type* BeginPtr() noexcept { return IsHeap() ? HeapBeginPtr() : SSOBeginPtr(); }
		inline const value_type* BeginPtr() const noexcept { return IsHeap() ? HeapBeginPtr() : SSOBeginPtr(); }

		inline value_type* HeapEndPtr() noexcept { return heap.m_begin + heap.m_size; }
		inline const value_type* HeapEndPtr() const noexcept { return heap.m_begin + heap.m_size; }

		inline value_type* SSOEndPtr() noexcept { return sso.m_data + GetSSOSize(); }
		inline const value_type* SSOEndPtr() const noexcept { return sso.m_data + GetSSOSize(); }

		// Points to end of character stream, *ptr == '0'
		inline value_type* EndPtr() noexcept { return IsHeap() ? HeapEndPtr() : SSOEndPtr(); }
		inline const value_type* EndPtr() const noexcept { return IsHeap() ? HeapEndPtr() : SSOEndPtr(); }

		inline value_type* HeapCapacityPtr() noexcept { return heap.m_begin + GetHeapCapacity(); }
		inline const value_type* HeapCapacityPtr() const noexcept { return heap.m_begin + GetHeapCapacity(); }

		inline value_type* SSOCapacityPtr() noexcept { return sso.m_data + SSOLayout::SSO_CAPACITY; }
		inline const value_type* SSOCapacityPtr() const noexcept { return sso.m_data + SSOLayout::SSO_CAPACITY; }

		// Points to end of the buffer at the terminating '0', *ptr == '0' <- only true when size() == capacity()
		inline value_type* CapacityPtr() noexcept { return IsHeap() ? HeapCapacityPtr() : SSOCapacityPtr(); }
		inline const value_type* CapacityPtr() const noexcept { return IsHeap() ? HeapCapacityPtr() : SSOCapacityPtr(); }

		inline void SetHeapBeginPtr(value_type* pBegin) noexcept { heap.m_begin = pBegin; }

		inline void SetHeapCapacity(size_type cap) noexcept
		{
#ifdef BIG_ENDIAN_NOT_IMPLEMENTED
			heap.m_capacity = (cap << 1) | kHeapMask;
#else
			heap.m_capacity = (cap | kHeapMask);
#endif
		}

		inline size_type GetHeapCapacity() const noexcept
		{
#ifdef BIG_ENDIAN_NOT_IMPLEMENTED
			return (heap.m_capacity >> 1);
#else
			return (heap.m_capacity & ~kHeapMask);
#endif
		}

		inline void Copy(Layout& dst, const Layout& src) noexcept { dst.raw = src.raw; }
		inline void Move(Layout& dst, Layout& src) noexcept { std::swap(dst.raw, src.raw); }
		inline void Swap(Layout& a, Layout& b) noexcept { std::swap(a.raw, b.raw); }

		inline void ResetToSSO() noexcept { *SSOBeginPtr() = 0; SetSSOSize(0); }
	};

	CompressedPair<Layout, allocator_type> m_pair;

	inline Layout& internalLayout() noexcept { return m_pair.First(); }
	inline const Layout& internalLayout() const noexcept { return m_pair.First(); }
	inline allocator_type& internalAllocator() noexcept { return m_pair.Second(); }
	inline const allocator_type& internalAllocator() const noexcept { return m_pair.Second(); }

public:
	// Constructor, destructor
	BasicString() noexcept;
	explicit BasicString(const allocator_type& allocator) noexcept;
	BasicString(const this_type& x, size_type position, size_type n = npos);
	BasicString(const value_type* p, size_type n, const allocator_type& allocator = allocator_type());
	BasicString(const value_type* p, const allocator_type& allocator = allocator_type());
	BasicString(size_type n, value_type c, const allocator_type& allocator = allocator_type());
	BasicString(const this_type& x, const allocator_type& allocator = allocator_type());
	BasicString(const value_type* pBegin, const value_type* pEnd, const allocator_type& allocator = allocator_type());
	BasicString(CtorDoNotInitialize, size_type n, const allocator_type& allocator = allocator_type());
	BasicString(std::initializer_list<value_type> init, const allocator_type& allocator = allocator_type());

	BasicString(this_type&& x) noexcept;
	BasicString(this_type&& x, const allocator_type& allocator);

	explicit BasicString(const view_type& sv, const allocator_type& allocator = allocator_type());
	BasicString(const view_type& sv, size_type position, size_type n, const allocator_type& allocator = allocator_type());

	template <typename OtherCharType>
	BasicString(CtorConvert, const OtherCharType* p, const allocator_type& allocator = allocator_type());

	template <typename OtherCharType>
	BasicString(CtorConvert, const OtherCharType* p, size_type n, const allocator_type& allocator = allocator_type());

	template <typename OtherStringType> // Unfortunately we need the CtorConvert here because otherwise this function would collide with the value_type* constructor.
	BasicString(CtorConvert, const OtherStringType& x);

	~BasicString();

	// Allocator
	const allocator_type& get_allocator() const noexcept;
	allocator_type& get_allocator() noexcept;
	void set_allocator(const allocator_type& allocator);

	// Implicit conversion operator
	operator BasicStringView<T>() const noexcept;

	// Operator=
	this_type& operator=(const this_type& x);
	this_type& operator=(const value_type* p);
	this_type& operator=(value_type c);
	this_type& operator=(std::initializer_list<value_type> ilist);
	this_type& operator=(view_type v);
	this_type& operator=(this_type&& x);

	void swap(this_type& x); // TODO(c++17): noexcept(allocator_traits<Allocator>::propagate_on_container_swap::value || allocator_traits<Allocator>::is_always_equal::value);

	// Assignment operations
	this_type& assign(const this_type& x);
	this_type& assign(const this_type& x, size_type position, size_type n = npos);
	this_type& assign(const value_type* p, size_type n);
	this_type& assign(const value_type* p);
	this_type& assign(size_type n, value_type c);
	this_type& assign(const value_type* pBegin, const value_type* pEnd);
	this_type& assign(this_type&& x); // TODO(c++17): noexcept(allocator_traits<Allocator>::propagate_on_container_move_assignment::value || allocator_traits<Allocator>::is_always_equal::value);
	this_type& assign(std::initializer_list<value_type>);

	template <typename OtherCharType>
	this_type& assign_convert(const OtherCharType* p);

	template <typename OtherCharType>
	this_type& assign_convert(const OtherCharType* p, size_type n);

	template <typename OtherStringType>
	this_type& assign_convert(const OtherStringType& x);

	// Iterators.
	iterator       begin() noexcept;                 // Expanded in source code as: mpBegin
	const_iterator begin() const noexcept;           // Expanded in source code as: mpBegin
	const_iterator cbegin() const noexcept;

	iterator       end() noexcept;                   // Expanded in source code as: mpEnd
	const_iterator end() const noexcept;             // Expanded in source code as: mpEnd
	const_iterator cend() const noexcept;

	reverse_iterator       rbegin() noexcept;
	const_reverse_iterator rbegin() const noexcept;
	const_reverse_iterator crbegin() const noexcept;

	reverse_iterator       rend() noexcept;
	const_reverse_iterator rend() const noexcept;
	const_reverse_iterator crend() const noexcept;


	// Size-related functionality
	bool      empty() const noexcept;
	size_type size() const noexcept;
	size_type length() const noexcept;
	size_type max_size() const noexcept;
	size_type capacity() const noexcept;
	void      resize(size_type n, value_type c);
	void      resize(size_type n);
	void      reserve(size_type = 0);
	void      set_capacity(size_type n = npos); // Revises the capacity to the user-specified value. Resizes the container to match the capacity if the requested capacity n is less than the current size. If n == npos then the capacity is reallocated (if necessary) such that capacity == size.
	void      force_size(size_type n);          // Unilaterally moves the string end position (mpEnd) to the given location. Useful for when the user writes into the string via some extenal means such as C strcpy or sprintf. This allows for more efficient use than using resize to achieve this.
	void shrink_to_fit();

	// Raw access
	const value_type* data() const  noexcept;
	value_type* data()        noexcept;
	const value_type* c_str() const noexcept;

	// Element access
	reference       operator[](size_type n);
	const_reference operator[](size_type n) const;
	reference       at(size_type n);
	const_reference at(size_type n) const;
	reference       front();
	const_reference front() const;
	reference       back();
	const_reference back() const;

	// Append operations
	this_type& operator+=(const this_type& x);
	this_type& operator+=(const value_type* p);
	this_type& operator+=(value_type c);

	this_type& append(const this_type& x);
	this_type& append(const this_type& x, size_type position, size_type n = npos);
	this_type& append(const value_type* p, size_type n);
	this_type& append(const value_type* p);
	this_type& append(size_type n, value_type c);
	this_type& append(const value_type* pBegin, const value_type* pEnd);

	template <typename OtherCharType>
	this_type& append_convert(const OtherCharType* p);

	template <typename OtherCharType>
	this_type& append_convert(const OtherCharType* p, size_type n);

	template <typename OtherStringType>
	this_type& append_convert(const OtherStringType& x);

	void push_back(value_type c);
	void pop_back();

	// Insertion operations
	this_type& insert(size_type position, const this_type& x);
	this_type& insert(size_type position, const this_type& x, size_type beg, size_type n);
	this_type& insert(size_type position, const value_type* p, size_type n);
	this_type& insert(size_type position, const value_type* p);
	this_type& insert(size_type position, size_type n, value_type c);
	iterator   insert(const_iterator p, value_type c);
	iterator   insert(const_iterator p, size_type n, value_type c);
	iterator   insert(const_iterator p, const value_type* pBegin, const value_type* pEnd);
	iterator   insert(const_iterator p, std::initializer_list<value_type>);

	// Erase operations
	this_type& erase(size_type position = 0, size_type n = npos);
	iterator         erase(const_iterator p);
	iterator         erase(const_iterator pBegin, const_iterator pEnd);
	reverse_iterator erase(reverse_iterator position);
	reverse_iterator erase(reverse_iterator first, reverse_iterator last);
	void             clear() noexcept;

	// Detach memory
	pointer detach() noexcept;

	// Replacement operations
	this_type& replace(size_type position, size_type n, const this_type& x);
	this_type& replace(size_type pos1, size_type n1, const this_type& x, size_type pos2, size_type n2 = npos);
	this_type& replace(size_type position, size_type n1, const value_type* p, size_type n2);
	this_type& replace(size_type position, size_type n1, const value_type* p);
	this_type& replace(size_type position, size_type n1, size_type n2, value_type c);
	this_type& replace(const_iterator first, const_iterator last, const this_type& x);
	this_type& replace(const_iterator first, const_iterator last, const value_type* p, size_type n);
	this_type& replace(const_iterator first, const_iterator last, const value_type* p);
	this_type& replace(const_iterator first, const_iterator last, size_type n, value_type c);
	this_type& replace(const_iterator first, const_iterator last, const value_type* pBegin, const value_type* pEnd);
	size_type   copy(value_type* p, size_type n, size_type position = 0) const;

	// Find operations
	size_type find(const this_type& x, size_type position = 0) const noexcept;
	size_type find(const value_type* p, size_type position = 0) const;
	size_type find(const value_type* p, size_type position, size_type n) const;
	size_type find(value_type c, size_type position = 0) const noexcept;

	// Reverse find operations
	size_type rfind(const this_type& x, size_type position = npos) const noexcept;
	size_type rfind(const value_type* p, size_type position = npos) const;
	size_type rfind(const value_type* p, size_type position, size_type n) const;
	size_type rfind(value_type c, size_type position = npos) const noexcept;

	// Find first-of operations
	size_type find_first_of(const this_type& x, size_type position = 0) const noexcept;
	size_type find_first_of(const value_type* p, size_type position = 0) const;
	size_type find_first_of(const value_type* p, size_type position, size_type n) const;
	size_type find_first_of(value_type c, size_type position = 0) const noexcept;

	// Find last-of operations
	size_type find_last_of(const this_type& x, size_type position = npos) const noexcept;
	size_type find_last_of(const value_type* p, size_type position = npos) const;
	size_type find_last_of(const value_type* p, size_type position, size_type n) const;
	size_type find_last_of(value_type c, size_type position = npos) const noexcept;

	// Find first not-of operations
	size_type find_first_not_of(const this_type& x, size_type position = 0) const noexcept;
	size_type find_first_not_of(const value_type* p, size_type position = 0) const;
	size_type find_first_not_of(const value_type* p, size_type position, size_type n) const;
	size_type find_first_not_of(value_type c, size_type position = 0) const noexcept;

	// Find last not-of operations
	size_type find_last_not_of(const this_type& x, size_type position = npos) const noexcept;
	size_type find_last_not_of(const value_type* p, size_type position = npos) const;
	size_type find_last_not_of(const value_type* p, size_type position, size_type n) const;
	size_type find_last_not_of(value_type c, size_type position = npos) const noexcept;

	// Substring functionality
	this_type substr(size_type position = 0, size_type n = npos) const;

	bool contains(const this_type& x) const noexcept;
	bool contains(const value_type* p) const;
	bool contains(BasicStringView<T> p) const;

	// Comparison operations
	int        compare(const this_type& x) const noexcept;
	int        compare(size_type pos1, size_type n1, const this_type& x) const;
	int        compare(size_type pos1, size_type n1, const this_type& x, size_type pos2, size_type n2) const;
	int        compare(const value_type* p) const;
	int        compare(size_type pos1, size_type n1, const value_type* p) const;
	int        compare(size_type pos1, size_type n1, const value_type* p, size_type n2) const;
	static int compare(const value_type* pBegin1, const value_type* pEnd1, const value_type* pBegin2, const value_type* pEnd2);

	// Case-insensitive comparison functions. Not part of C++ this_type. Only ASCII-level locale functionality is supported. Thus this is not suitable for localization purposes.
	int        comparei(const this_type& x) const noexcept;
	int        comparei(const value_type* p) const;
	static int comparei(const value_type* pBegin1, const value_type* pEnd1, const value_type* pBegin2, const value_type* pEnd2);

	// Misc functionality, not part of C++ this_type.
	void         make_lower();
	void         make_upper();
	void         ltrim();
	void         rtrim();
	void         trim();
	void         ltrim(const value_type* p);
	void         rtrim(const value_type* p);
	void         trim(const value_type* p);
	this_type    left(size_type n) const;
	this_type    right(size_type n) const;

	bool validate() const noexcept;
	int  validate_iterator(const_iterator i) const noexcept;


protected:
	// Helper functions for initialization/insertion operations.
	value_type* DoAllocate(size_type n);
	void        DoFree(value_type* p, size_type n);
	size_type   GetNewCapacity(size_type currentCapacity);
	size_type   GetNewCapacity(size_type currentCapacity, size_type minimumGrowSize);
	void        AllocateSelf();
	void        AllocateSelf(size_type n);
	void        DeallocateSelf();
	iterator    InsertInternal(const_iterator p, value_type c);
	void        RangeInitialize(const value_type* pBegin, const value_type* pEnd);
	void        RangeInitialize(const value_type* pBegin);
	void        SizeInitialize(size_type n, value_type c);

	bool        IsSSO() const noexcept;

	void        ThrowLengthException() const;
	void        ThrowRangeException() const;
	void        ThrowInvalidArgumentException() const;
};

template<typename T, typename Allocator /*= DefaultHeapAllocator*/>
bool BasicString<T, Allocator>::contains(BasicStringView<T> p) const
{
	auto it = find(p.data());
	return it != npos;
}

template<typename T, typename Allocator /*= DefaultHeapAllocator*/>
bool BasicString<T, Allocator>::contains(const value_type* p) const
{
	auto it = find(p);
	return it != npos;
}

template<typename T, typename Allocator /*= DefaultHeapAllocator*/>
bool BasicString<T, Allocator>::contains(const this_type& x) const noexcept
{
	auto it = find(x);
	return it != npos;
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator>::BasicString() noexcept
	: m_pair(allocator_type())
{
	AllocateSelf();
}


template <typename T, typename Allocator>
inline BasicString<T, Allocator>::BasicString(const allocator_type& allocator) noexcept
	: m_pair(allocator)
{
	AllocateSelf();
}

template <typename T, typename Allocator>
BasicString<T, Allocator>::BasicString(const this_type& x, const allocator_type& allocator)
	: m_pair(allocator)
{
	RangeInitialize(x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
}


template <typename T, typename Allocator>
template <typename OtherStringType>
inline BasicString<T, Allocator>::BasicString(CtorConvert, const OtherStringType& x)
{
	AllocateSelf();
	append_convert(x.c_str(), x.length());
}

template <typename T, typename Allocator>
BasicString<T, Allocator>::BasicString(const this_type& x, size_type position, size_type n)
	: m_pair(x.get_allocator())
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(position > x.internalLayout().GetSize())) // 21.4.2 p4
	{
		ThrowRangeException();
		AllocateSelf();
	}
	else
		RangeInitialize(
			x.internalLayout().BeginPtr() + position,
			x.internalLayout().BeginPtr() + position + std::min(n, x.internalLayout().GetSize() - position));
#else
	RangeInitialize(
		x.internalLayout().BeginPtr() + position,
		x.internalLayout().BeginPtr() + position + std::min(n, x.internalLayout().GetSize() - position));
#endif
}


template <typename T, typename Allocator>
inline BasicString<T, Allocator>::BasicString(const value_type* p, size_type n, const allocator_type& allocator)
	: m_pair(allocator)
{
	RangeInitialize(p, p + n);
}


template <typename T, typename Allocator>
inline BasicString<T, Allocator>::BasicString(const view_type& sv, const allocator_type& allocator)
	: BasicString(sv.data(), static_cast<size_type>(sv.size()), allocator)
{}


template <typename T, typename Allocator>
inline BasicString<T, Allocator>::BasicString(const view_type& sv, size_type position, size_type n, const allocator_type& allocator)
	: BasicString(sv.substr(position, n), allocator)
{}


template <typename T, typename Allocator>
template <typename OtherCharType>
inline BasicString<T, Allocator>::BasicString(CtorConvert, const OtherCharType* p, const allocator_type& allocator)
	: m_pair(allocator)
{
	AllocateSelf();    // In this case we are converting from one string encoding to another, and we
	append_convert(p); // implement this in the simplest way, by simply default-constructing and calling assign.
}


template <typename T, typename Allocator>
template <typename OtherCharType>
inline BasicString<T, Allocator>::BasicString(CtorConvert, const OtherCharType* p, size_type n, const allocator_type& allocator)
	: m_pair(allocator)
{
	AllocateSelf();         // In this case we are converting from one string encoding to another, and we
	append_convert(p, n);   // implement this in the simplest way, by simply default-constructing and calling assign.
}


template <typename T, typename Allocator>
inline BasicString<T, Allocator>::BasicString(const value_type* p, const allocator_type& allocator)
	: m_pair(allocator)
{
	RangeInitialize(p);
}


template <typename T, typename Allocator>
inline BasicString<T, Allocator>::BasicString(size_type n, value_type c, const allocator_type& allocator)
	: m_pair(allocator)
{
	SizeInitialize(n, c);
}


template <typename T, typename Allocator>
inline BasicString<T, Allocator>::BasicString(const value_type* pBegin, const value_type* pEnd, const allocator_type& allocator)
	: m_pair(allocator)
{
	RangeInitialize(pBegin, pEnd);
}


// CtorDoNotInitialize exists so that we can create a version that allocates but doesn't
// initialize but also doesn't collide with any other constructor declaration.
template <typename T, typename Allocator>
BasicString<T, Allocator>::BasicString(CtorDoNotInitialize /*unused*/, size_type n, const allocator_type& allocator)
	: m_pair(allocator)
{
	// Note that we do not call SizeInitialize here.
	AllocateSelf(n);
	internalLayout().SetSize(0);
	*internalLayout().EndPtr() = 0;
}

template <typename T, typename Allocator>
BasicString<T, Allocator>::BasicString(std::initializer_list<value_type> init, const allocator_type& allocator)
	: m_pair(allocator)
{
	RangeInitialize(init.begin(), init.end());
}


template <typename T, typename Allocator>
BasicString<T, Allocator>::BasicString(this_type&& x) noexcept
	: m_pair(x.get_allocator())
{
	internalLayout() = std::move(x.internalLayout());
	x.AllocateSelf();
}


template <typename T, typename Allocator>
BasicString<T, Allocator>::BasicString(this_type&& x, const allocator_type& allocator)
	: m_pair(allocator)
{
	if (get_allocator() == x.get_allocator()) // If we can borrow from x...
	{
		internalLayout() = std::move(x.internalLayout());
		x.AllocateSelf();
	}
	else if (x.internalLayout().BeginPtr())
	{
		RangeInitialize(x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
		// Let x destruct its own items.
	}
}


template <typename T, typename Allocator>
inline BasicString<T, Allocator>::~BasicString()
{
	DeallocateSelf();
}


template <typename T, typename Allocator>
inline const typename BasicString<T, Allocator>::allocator_type&
BasicString<T, Allocator>::get_allocator() const noexcept
{
	return internalAllocator();
}


template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::allocator_type&
BasicString<T, Allocator>::get_allocator() noexcept
{
	return internalAllocator();
}


template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::set_allocator(const allocator_type& allocator)
{
	if (internalLayout().IsHeap() && get_allocator() != allocator)
		VT_ASSERT_MSG(false, "BasicString::set_allocator -- cannot change allocator after allocations have been made.");
	get_allocator() = allocator;
}

template<typename T, typename Allocator>
inline BasicString<T, Allocator>::operator BasicStringView<T>() const noexcept
{
	return BasicStringView<T>(data(), size());
}

template <typename T, typename Allocator>
inline const typename BasicString<T, Allocator>::value_type*
BasicString<T, Allocator>::data()  const noexcept
{
	return internalLayout().BeginPtr();
}

template <typename T, typename Allocator>
inline const typename BasicString<T, Allocator>::value_type*
BasicString<T, Allocator>::c_str() const noexcept
{
	return internalLayout().BeginPtr();
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::value_type*
BasicString<T, Allocator>::data() noexcept
{
	return internalLayout().BeginPtr();
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::iterator
BasicString<T, Allocator>::begin() noexcept
{
	return internalLayout().BeginPtr();
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::iterator
BasicString<T, Allocator>::end() noexcept
{
	return internalLayout().EndPtr();
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::const_iterator
BasicString<T, Allocator>::begin() const noexcept
{
	return internalLayout().BeginPtr();
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::const_iterator
BasicString<T, Allocator>::cbegin() const noexcept
{
	return internalLayout().BeginPtr();
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::const_iterator
BasicString<T, Allocator>::end() const noexcept
{
	return internalLayout().EndPtr();
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::const_iterator
BasicString<T, Allocator>::cend() const noexcept
{
	return internalLayout().EndPtr();
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::reverse_iterator
BasicString<T, Allocator>::rbegin() noexcept
{
	return reverse_iterator(internalLayout().EndPtr());
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::reverse_iterator
BasicString<T, Allocator>::rend() noexcept
{
	return reverse_iterator(internalLayout().BeginPtr());
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::const_reverse_iterator
BasicString<T, Allocator>::rbegin() const noexcept
{
	return const_reverse_iterator(internalLayout().EndPtr());
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::const_reverse_iterator
BasicString<T, Allocator>::crbegin() const noexcept
{
	return const_reverse_iterator(internalLayout().EndPtr());
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::const_reverse_iterator
BasicString<T, Allocator>::rend() const noexcept
{
	return const_reverse_iterator(internalLayout().BeginPtr());
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::const_reverse_iterator
BasicString<T, Allocator>::crend() const noexcept
{
	return const_reverse_iterator(internalLayout().BeginPtr());
}

template <typename T, typename Allocator>
inline bool BasicString<T, Allocator>::empty() const noexcept
{
	return (internalLayout().GetSize() == 0);
}

template <typename T, typename Allocator>
inline bool BasicString<T, Allocator>::IsSSO() const noexcept
{
	return internalLayout().IsSSO();
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::size() const noexcept
{
	return internalLayout().GetSize();
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::length() const noexcept
{
	return internalLayout().GetSize();
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::max_size() const noexcept
{
	return kMaxSize;
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::capacity() const noexcept
{
	if (internalLayout().IsHeap())
	{
		return internalLayout().GetHeapCapacity();
	}
	return SSOLayout::SSO_CAPACITY;
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::const_reference
BasicString<T, Allocator>::operator[](size_type n) const
{
#if EASTL_ASSERT_ENABLED // We allow the user to reference the trailing 0 char without asserting. Perhaps we shouldn't.
	if (EASTL_UNLIKELY(n > internalLayout().GetSize()))
		EASTL_FAIL_MSG("BasicString::operator[] -- out of range");
#endif

	return internalLayout().BeginPtr()[n]; // Sometimes done as *(mpBegin + n)
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::reference
BasicString<T, Allocator>::operator[](size_type n)
{
#if EASTL_ASSERT_ENABLED // We allow the user to reference the trailing 0 char without asserting. Perhaps we shouldn't.
	if (EASTL_UNLIKELY(n > internalLayout().GetSize()))
		EASTL_FAIL_MSG("BasicString::operator[] -- out of range");
#endif

	return internalLayout().BeginPtr()[n]; // Sometimes done as *(mpBegin + n)
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::this_type& BasicString<T, Allocator>::operator=(const this_type& x)
{
	if (&x != this)
	{
		assign(x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
	}
	return *this;
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::this_type& BasicString<T, Allocator>::operator=(const value_type* p)
{
	return assign(p, p + StringAlgorithm::Strlen(p));
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::this_type& BasicString<T, Allocator>::operator=(value_type c)
{
	return assign((size_type)1, c);
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::this_type& BasicString<T, Allocator>::operator=(this_type&& x)
{
	return assign(std::move(x));
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::this_type& BasicString<T, Allocator>::operator=(std::initializer_list<value_type> ilist)
{
	return assign(ilist.begin(), ilist.end());
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::this_type& BasicString<T, Allocator>::operator=(view_type v)
{
	return assign(v.data(), static_cast<this_type::size_type>(v.size()));
}

template <typename T, typename Allocator>
void BasicString<T, Allocator>::resize(size_type n, value_type c)
{
	const size_type s = internalLayout().GetSize();

	if (n < s)
		erase(internalLayout().BeginPtr() + n, internalLayout().EndPtr());
	else if (n > s)
		append(n - s, c);
}

template <typename T, typename Allocator>
void BasicString<T, Allocator>::resize(size_type n)
{
	// C++ BasicString specifies that resize(n) is equivalent to resize(n, value_type()).
	// For built-in types, value_type() is the same as zero (value_type(0)).
	// We can improve the efficiency (especially for long strings) of this
	// string class by resizing without assigning to anything.

	const size_type s = internalLayout().GetSize();

	if (n < s)
		erase(internalLayout().BeginPtr() + n, internalLayout().EndPtr());
	else if (n > s)
	{
		append(n - s, value_type());
	}
}

template <typename T, typename Allocator>
void BasicString<T, Allocator>::reserve(size_type n)
{
#if EASTL_STRING_OPT_LENGTH_ERRORS
	if (EASTL_UNLIKELY(n > max_size()))
		ThrowLengthException();
#endif

	// C++20 says if the passed in capacity is less than the current capacity we do not shrink
	// If new_cap is less than or equal to the current capacity(), there is no effect.
	// http://en.cppreference.com/w/cpp/string/BasicString/reserve

	n = std::max(n, internalLayout().GetSize()); // Calculate the new capacity, which needs to be >= container size.

	if (n > capacity())
		set_capacity(n);
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::shrink_to_fit()
{
	set_capacity(internalLayout().GetSize());
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::set_capacity(size_type n)
{
	if (n == npos)
		// If the user wants to set the capacity to equal the current size...
		// '-1' because we pretend that we didn't allocate memory for the terminating 0.
		n = internalLayout().GetSize();
	else if (n < internalLayout().GetSize())
	{
		internalLayout().SetSize(n);
		*internalLayout().EndPtr() = 0;
	}

	if ((n < capacity() && internalLayout().IsHeap()) || (n > capacity()))
	{
		// In here the string is transition from heap->heap, heap->sso or sso->heap

		if (n > 0)
		{

			if (n <= SSOLayout::SSO_CAPACITY)
			{
				// heap->sso
				// A heap based layout wants to reduce its size to within sso capacity
				// An sso layout wanting to reduce its capacity will not get in here
				pointer pOldBegin = internalLayout().BeginPtr();
				const size_type nOldCap = internalLayout().GetHeapCapacity();

				StringAlgorithm::StringUninitializedCopy(pOldBegin, pOldBegin + n, internalLayout().SSOBeginPtr());
				internalLayout().SetSSOSize(n);
				*internalLayout().SSOEndPtr() = 0;

				DoFree(pOldBegin, nOldCap + 1);

				return;
			}

			pointer pNewBegin = DoAllocate(n + 1); // We need the + 1 to accomodate the trailing 0.
			size_type nSavedSize = internalLayout().GetSize(); // save the size in case we transition from sso->heap

			pointer pNewEnd = StringAlgorithm::StringUninitializedCopy(internalLayout().BeginPtr(), internalLayout().EndPtr(), pNewBegin);
			*pNewEnd = 0;

			DeallocateSelf();

			internalLayout().SetHeapBeginPtr(pNewBegin);
			internalLayout().SetHeapCapacity(n);
			internalLayout().SetHeapSize(nSavedSize);
		}
		else
		{
			DeallocateSelf();
			AllocateSelf();
		}
	}
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::force_size(size_type n)
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(n > capacity()))
		ThrowRangeException();
#elif EASTL_ASSERT_ENABLED
	if (EASTL_UNLIKELY(n > capacity()))
		EASTL_FAIL_MSG("BasicString::force_size -- out of range");
#endif

	internalLayout().SetSize(n);
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::clear() noexcept
{
	internalLayout().SetSize(0);
	*internalLayout().BeginPtr() = value_type(0);
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::pointer
BasicString<T, Allocator>::detach() noexcept
{
	// The detach function is an extension function which simply forgets the
	// owned pointer. It doesn't free it but rather assumes that the user
	// does. If the string is utilizing the short-string-optimization when a
	// detach is requested, a copy of the string into a seperate memory
	// allocation occurs and the owning pointer is given to the user who is
	// responsible for freeing the memory.

	pointer pDetached = nullptr;

	if (internalLayout().IsSSO())
	{
		const size_type n = internalLayout().GetSize() + 1; // +1' so that we have room for the terminating 0.
		pDetached = DoAllocate(n);
		pointer pNewEnd = StringAlgorithm::StringUninitializedCopy(internalLayout().BeginPtr(), internalLayout().EndPtr(), pDetached);
		*pNewEnd = 0;
	}
	else
	{
		pDetached = internalLayout().BeginPtr();
	}

	AllocateSelf(); // reset to string to empty
	return pDetached;
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::const_reference
BasicString<T, Allocator>::at(size_type n) const
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(n >= internalLayout().GetSize()))
		ThrowRangeException();
#elif EASTL_ASSERT_ENABLED                  // We assert if the user references the trailing 0 char.
	if (EASTL_UNLIKELY(n >= internalLayout().GetSize()))
		EASTL_FAIL_MSG("BasicString::at -- out of range");
#endif

	return internalLayout().BeginPtr()[n];
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::reference
BasicString<T, Allocator>::at(size_type n)
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(n >= internalLayout().GetSize()))
		ThrowRangeException();
#elif EASTL_ASSERT_ENABLED                  // We assert if the user references the trailing 0 char.
	if (EASTL_UNLIKELY(n >= internalLayout().GetSize()))
		EASTL_FAIL_MSG("BasicString::at -- out of range");
#endif

	return internalLayout().BeginPtr()[n];
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::reference
BasicString<T, Allocator>::front()
{
#if EASTL_ASSERT_ENABLED && EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
	if (EASTL_UNLIKELY(internalLayout().GetSize() == 0)) // We assert if the user references the trailing 0 char.
		EASTL_FAIL_MSG("BasicString::front -- empty string");
#else
	// We allow the user to reference the trailing 0 char without asserting.
#endif

	return *internalLayout().BeginPtr();
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::const_reference
BasicString<T, Allocator>::front() const
{
#if EASTL_ASSERT_ENABLED && EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
	if (EASTL_UNLIKELY(internalLayout().GetSize() == 0)) // We assert if the user references the trailing 0 char.
		EASTL_FAIL_MSG("BasicString::front -- empty string");
#else
	// We allow the user to reference the trailing 0 char without asserting.
#endif

	return *internalLayout().BeginPtr();
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::reference
BasicString<T, Allocator>::back()
{
#if EASTL_ASSERT_ENABLED && EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
	if (EASTL_UNLIKELY(internalLayout().GetSize() == 0)) // We assert if the user references the trailing 0 char.
		EASTL_FAIL_MSG("BasicString::back -- empty string");
#else
	// We allow the user to reference the trailing 0 char without asserting.
#endif

	return *(internalLayout().EndPtr() - 1);
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::const_reference
BasicString<T, Allocator>::back() const
{
#if EASTL_ASSERT_ENABLED && EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
	if (EASTL_UNLIKELY(internalLayout().GetSize() == 0)) // We assert if the user references the trailing 0 char.
		EASTL_FAIL_MSG("BasicString::back -- empty string");
#else
	// We allow the user to reference the trailing 0 char without asserting.
#endif

	return *(internalLayout().EndPtr() - 1);
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator>& BasicString<T, Allocator>::operator+=(const this_type& x)
{
	return append(x);
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator>& BasicString<T, Allocator>::operator+=(const value_type* p)
{
	return append(p);
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator>& BasicString<T, Allocator>::operator+=(value_type c)
{
	push_back(c);
	return *this;
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator>& BasicString<T, Allocator>::append(const this_type& x)
{
	return append(x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator>& BasicString<T, Allocator>::append(const this_type& x, size_type position, size_type n)
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(position >= x.internalLayout().GetSize())) // position must be < x.mpEnd, but position + n may be > mpEnd.
		ThrowRangeException();
#endif

	return append(x.internalLayout().BeginPtr() + position,
				  x.internalLayout().BeginPtr() + position + std::min(n, x.internalLayout().GetSize() - position));
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator>& BasicString<T, Allocator>::append(const value_type* p, size_type n)
{
	return append(p, p + n);
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator>& BasicString<T, Allocator>::append(const value_type* p)
{
	return append(p, p + StringAlgorithm::Strlen(p));
}

template <typename T, typename Allocator>
template <typename OtherCharType>
BasicString<T, Allocator>& BasicString<T, Allocator>::append_convert(const OtherCharType* pOther)
{
	return append_convert(pOther, (size_type)StringAlgorithm::Strlen(pOther));
}

template <typename T, typename Allocator>
template <typename OtherStringType>
BasicString<T, Allocator>& BasicString<T, Allocator>::append_convert(const OtherStringType& x)
{
	return append_convert(x.c_str(), x.length());
}

template <typename T, typename Allocator>
template <typename OtherCharType>
BasicString<T, Allocator>& BasicString<T, Allocator>::append_convert(const OtherCharType* pOther, size_type n)
{
	// Question: What do we do in the case that we have an illegally encoded source string?
	// This can happen with UTF8 strings. Do we throw an exception or do we ignore the input?
	// One argument is that it's not a string class' job to handle the security aspects of a
	// program and the higher level application code should be verifying UTF8 string validity,
	// and thus we should do the friendly thing and ignore the invalid characters as opposed
	// to making the user of this function handle exceptions that are easily forgotten.

	const size_t         kBufferSize = 512;
	value_type           selfBuffer[kBufferSize];   // This assumes that value_type is one of char8_t, char16_t, char32_t, or wchar_t. Or more importantly, a type with a trivial constructor and destructor.
	value_type* const    selfBufferEnd = selfBuffer + kBufferSize;
	const OtherCharType* pOtherEnd = pOther + n;

	while (pOther != pOtherEnd)
	{
		value_type* pSelfBufferCurrent = selfBuffer;
		DecodePart(pOther, pOtherEnd, pSelfBufferCurrent, selfBufferEnd);   // Write pOther to pSelfBuffer, converting encoding as we go. We currently ignore the return value, as we don't yet have a plan for handling encoding errors.
		append(selfBuffer, pSelfBufferCurrent);
	}

	return *this;
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::append(size_type n, value_type c)
{
	if (n > 0)
	{
		const size_type nSize = internalLayout().GetSize();
		const size_type nCapacity = capacity();

		if ((nSize + n) > nCapacity)
			reserve(GetNewCapacity(nCapacity, (nSize + n) - nCapacity));

		pointer pNewEnd = StringAlgorithm::StringUninitializedFillN(internalLayout().EndPtr(), n, c);
		*pNewEnd = 0;
		internalLayout().SetSize(nSize + n);
	}

	return *this;
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::append(const value_type* pBegin, const value_type* pEnd)
{
	if (pBegin != pEnd)
	{
		const size_type nOldSize = internalLayout().GetSize();
		const size_type n = (size_type)(pEnd - pBegin);
		const size_type nCapacity = capacity();
		const size_type nNewSize = nOldSize + n;

		if (nNewSize > nCapacity)
		{
			const size_type nLength = GetNewCapacity(nCapacity, nNewSize - nCapacity);

			pointer pNewBegin = DoAllocate(nLength + 1);

			pointer pNewEnd = StringAlgorithm::StringUninitializedCopy(internalLayout().BeginPtr(), internalLayout().EndPtr(), pNewBegin);
			pNewEnd = StringAlgorithm::StringUninitializedCopy(pBegin, pEnd, pNewEnd);
			*pNewEnd = 0;

			DeallocateSelf();
			internalLayout().SetHeapBeginPtr(pNewBegin);
			internalLayout().SetHeapCapacity(nLength);
			internalLayout().SetHeapSize(nNewSize);
		}
		else
		{
			pointer pNewEnd = StringAlgorithm::StringUninitializedCopy(pBegin, pEnd, internalLayout().EndPtr());
			*pNewEnd = 0;
			internalLayout().SetSize(nNewSize);
		}
	}

	return *this;
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::push_back(value_type c)
{
	append((size_type)1, c);
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::pop_back()
{
#if EASTL_ASSERT_ENABLED
	if (EASTL_UNLIKELY(internalLayout().GetSize() <= 0))
		EASTL_FAIL_MSG("BasicString::pop_back -- empty string");
#endif

	internalLayout().EndPtr()[-1] = value_type(0);
	internalLayout().SetSize(internalLayout().GetSize() - 1);
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator>& BasicString<T, Allocator>::assign(const this_type& x)
{
	// The C++11 Standard 21.4.6.3 p6 specifies that assign from this_type assigns contents only and not the allocator.
	return assign(x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator>& BasicString<T, Allocator>::assign(const this_type& x, size_type position, size_type n)
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(position > x.internalLayout().GetSize()))
		ThrowRangeException();
#endif

	// The C++11 Standard 21.4.6.3 p6 specifies that assign from this_type assigns contents only and not the allocator.
	return assign(
		x.internalLayout().BeginPtr() + position,
		x.internalLayout().BeginPtr() + position + std::min(n, x.internalLayout().GetSize() - position));
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator>& BasicString<T, Allocator>::assign(const value_type* p, size_type n)
{
	return assign(p, p + n);
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator>& BasicString<T, Allocator>::assign(const value_type* p)
{
	return assign(p, p + StringAlgorithm::Strlen(p));
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::assign(size_type n, value_type c)
{
	if (n <= internalLayout().GetSize())
	{
		StringAlgorithm::AssignN(internalLayout().BeginPtr(), n, c);
		erase(internalLayout().BeginPtr() + n, internalLayout().EndPtr());
	}
	else
	{
		StringAlgorithm::AssignN(internalLayout().BeginPtr(), internalLayout().GetSize(), c);
		append(n - internalLayout().GetSize(), c);
	}
	return *this;
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::assign(const value_type* pBegin, const value_type* pEnd)
{
	const size_type n = (size_type)(pEnd - pBegin);
	if (n <= internalLayout().GetSize())
	{
		if (n)
			memmove(internalLayout().BeginPtr(), pBegin, (size_t)n * sizeof(value_type));
		erase(internalLayout().BeginPtr() + n, internalLayout().EndPtr());
	}
	else
	{
		memmove(internalLayout().BeginPtr(), pBegin, (size_t)(internalLayout().GetSize()) * sizeof(value_type));
		append(pBegin + internalLayout().GetSize(), pEnd);
	}
	return *this;
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator>& BasicString<T, Allocator>::assign(std::initializer_list<value_type> ilist)
{
	return assign(ilist.begin(), ilist.end());
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator>& BasicString<T, Allocator>::assign(this_type&& x)
{
	std::swap(internalLayout(), x.internalLayout());
	std::swap(get_allocator(), x.get_allocator());

	return *this;
}

template <typename T, typename Allocator>
template <typename OtherCharType>
BasicString<T, Allocator>& BasicString<T, Allocator>::assign_convert(const OtherCharType* p)
{
	clear();
	append_convert(p);
	return *this;
}

template <typename T, typename Allocator>
template <typename OtherCharType>
BasicString<T, Allocator>& BasicString<T, Allocator>::assign_convert(const OtherCharType* p, size_type n)
{
	clear();
	append_convert(p, n);
	return *this;
}

template <typename T, typename Allocator>
template <typename OtherStringType>
BasicString<T, Allocator>& BasicString<T, Allocator>::assign_convert(const OtherStringType& x)
{
	clear();
	append_convert(x.data(), x.length());
	return *this;
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::insert(size_type position, const this_type& x)
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(position > internalLayout().GetSize()))
		ThrowRangeException();
#endif

#if EASTL_STRING_OPT_LENGTH_ERRORS
	if (EASTL_UNLIKELY(internalLayout().GetSize() > (max_size() - x.internalLayout().GetSize())))
		ThrowLengthException();
#endif

	insert(internalLayout().BeginPtr() + position, x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
	return *this;
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::insert(size_type position, const this_type& x, size_type beg, size_type n)
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY((position > internalLayout().GetSize()) || (beg > x.internalLayout().GetSize())))
		ThrowRangeException();
#endif

	size_type nLength = std::min(n, x.internalLayout().GetSize() - beg);

#if EASTL_STRING_OPT_LENGTH_ERRORS
	if (EASTL_UNLIKELY(internalLayout().GetSize() > (max_size() - nLength)))
		ThrowLengthException();
#endif

	insert(internalLayout().BeginPtr() + position, x.internalLayout().BeginPtr() + beg, x.internalLayout().BeginPtr() + beg + nLength);
	return *this;
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::insert(size_type position, const value_type* p, size_type n)
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(position > internalLayout().GetSize()))
		ThrowRangeException();
#endif

#if EASTL_STRING_OPT_LENGTH_ERRORS
	if (EASTL_UNLIKELY(internalLayout().GetSize() > (max_size() - n)))
		ThrowLengthException();
#endif

	insert(internalLayout().BeginPtr() + position, p, p + n);
	return *this;
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::insert(size_type position, const value_type* p)
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(position > internalLayout().GetSize()))
		ThrowRangeException();
#endif

	size_type nLength = (size_type)StringAlgorithm::Strlen(p);

#if EASTL_STRING_OPT_LENGTH_ERRORS
	if (EASTL_UNLIKELY(internalLayout().GetSize() > (max_size() - nLength)))
		ThrowLengthException();
#endif

	insert(internalLayout().BeginPtr() + position, p, p + nLength);
	return *this;
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::insert(size_type position, size_type n, value_type c)
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(position > internalLayout().GetSize()))
		ThrowRangeException();
#endif

#if EASTL_STRING_OPT_LENGTH_ERRORS
	if (EASTL_UNLIKELY(internalLayout().GetSize() > (max_size() - n)))
		ThrowLengthException();
#endif

	insert(internalLayout().BeginPtr() + position, n, c);
	return *this;
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::iterator
BasicString<T, Allocator>::insert(const_iterator p, value_type c)
{
	if (p == internalLayout().EndPtr())
	{
		push_back(c);
		return internalLayout().EndPtr() - 1;
	}
	return InsertInternal(p, c);
}

template <typename T, typename Allocator>
typename BasicString<T, Allocator>::iterator
BasicString<T, Allocator>::insert(const_iterator p, size_type n, value_type c)
{
	const difference_type nPosition = (p - internalLayout().BeginPtr()); // Save this because we might reallocate.

#if EASTL_ASSERT_ENABLED
	if (EASTL_UNLIKELY((p < internalLayout().BeginPtr()) || (p > internalLayout().EndPtr())))
		EASTL_FAIL_MSG("BasicString::insert -- invalid position");
#endif

	if (n) // If there is anything to insert...
	{
		if (internalLayout().GetRemainingCapacity() >= n) // If we have enough capacity...
		{
			const size_type nElementsAfter = (size_type)(internalLayout().EndPtr() - p);

			if (nElementsAfter >= n) // If there's enough space for the new chars between the insert position and the end...
			{
				// Ensure we save the size before we do the copy, as we might overwrite the size field with the NULL
				// terminator in the edge case where we are inserting enough characters to equal our capacity
				const size_type nSavedSize = internalLayout().GetSize();
				StringAlgorithm::StringUninitializedCopy((internalLayout().EndPtr() - n) + 1, internalLayout().EndPtr() + 1, internalLayout().EndPtr() + 1);
				internalLayout().SetSize(nSavedSize + n);
				memmove(const_cast<value_type*>(p) + n, p, (size_t)((nElementsAfter - n) + 1) * sizeof(value_type));
				StringAlgorithm::AssignN(const_cast<value_type*>(p), n, c);
			}
			else
			{
				pointer pOldEnd = internalLayout().EndPtr();
#if EASTL_EXCEPTIONS_ENABLED
				const size_type nOldSize = internalLayout().GetSize();
#endif
				StringAlgorithm::StringUninitializedFillN(internalLayout().EndPtr() + 1, n - nElementsAfter - 1, c);
				internalLayout().SetSize(internalLayout().GetSize() + (n - nElementsAfter));

#if EASTL_EXCEPTIONS_ENABLED
				try
				{
#endif
					// See comment in if block above
					const size_type nSavedSize = internalLayout().GetSize();
					StringAlgorithm::StringUninitializedCopy(p, pOldEnd + 1, internalLayout().EndPtr());
					internalLayout().SetSize(nSavedSize + nElementsAfter);
#if EASTL_EXCEPTIONS_ENABLED
				}
				catch (...)
				{
					internalLayout().SetSize(nOldSize);
					throw;
				}
#endif

				StringAlgorithm::AssignN(const_cast<value_type*>(p), nElementsAfter + 1, c);
			}
		}
		else
		{
			const size_type nOldSize = internalLayout().GetSize();
			const size_type nOldCap = capacity();
			const size_type nLength = GetNewCapacity(nOldCap, (nOldSize + n) - nOldCap);

			iterator pNewBegin = DoAllocate(nLength + 1);

			iterator pNewEnd = StringAlgorithm::StringUninitializedCopy(internalLayout().BeginPtr(), p, pNewBegin);
			pNewEnd = StringAlgorithm::StringUninitializedFillN(pNewEnd, n, c);
			pNewEnd = StringAlgorithm::StringUninitializedCopy(p, internalLayout().EndPtr(), pNewEnd);
			*pNewEnd = 0;

			DeallocateSelf();
			internalLayout().SetHeapBeginPtr(pNewBegin);
			internalLayout().SetHeapCapacity(nLength);
			internalLayout().SetHeapSize(nOldSize + n);
		}
	}

	return internalLayout().BeginPtr() + nPosition;
}

template <typename T, typename Allocator>
typename BasicString<T, Allocator>::iterator
BasicString<T, Allocator>::insert(const_iterator p, const value_type* pBegin, const value_type* pEnd)
{
	const difference_type nPosition = (p - internalLayout().BeginPtr()); // Save this because we might reallocate.

#if EASTL_ASSERT_ENABLED
	if (EASTL_UNLIKELY((p < internalLayout().BeginPtr()) || (p > internalLayout().EndPtr())))
		EASTL_FAIL_MSG("BasicString::insert -- invalid position");
#endif

	const size_type n = (size_type)(pEnd - pBegin);

	if (n)
	{
		const bool bCapacityIsSufficient = (internalLayout().GetRemainingCapacity() >= n);
		const bool bSourceIsFromSelf = ((pEnd >= internalLayout().BeginPtr()) && (pBegin <= internalLayout().EndPtr()));

		if (bSourceIsFromSelf && internalLayout().IsSSO())
		{
			// pBegin to pEnd will be <= this->GetSize(), so stackTemp will guaranteed be an SSO String
			// If we are inserting ourself into ourself and we are SSO, then on the recursive call we can
			// guarantee 0 or 1 allocation depending if we need to realloc
			// We don't do this for Heap strings as then this path may do 1 or 2 allocations instead of
			// only 1 allocation when we fall through to the last else case below
			const this_type stackTemp(pBegin, pEnd);
			return insert(p, stackTemp.data(), stackTemp.data() + stackTemp.size());
		}

		// If bSourceIsFromSelf is true, then we reallocate. This is because we are
		// inserting ourself into ourself and thus both the source and destination
		// be modified, making it rather tricky to attempt to do in place. The simplest
		// resolution is to reallocate. To consider: there may be a way to implement this
		// whereby we don't need to reallocate or can often avoid reallocating.
		if (bCapacityIsSufficient && !bSourceIsFromSelf)
		{
			const size_type nElementsAfter = (size_type)(internalLayout().EndPtr() - p);

			if (nElementsAfter >= n) // If there are enough characters between insert pos and end
			{
				// Ensure we save the size before we do the copy, as we might overwrite the size field with the NULL
				// terminator in the edge case where we are inserting enough characters to equal our capacity
				const size_type nSavedSize = internalLayout().GetSize();
				StringAlgorithm::StringUninitializedCopy((internalLayout().EndPtr() - n) + 1, internalLayout().EndPtr() + 1, internalLayout().EndPtr() + 1);
				internalLayout().SetSize(nSavedSize + n);
				memmove(const_cast<value_type*>(p) + n, p, (size_t)((nElementsAfter - n) + 1) * sizeof(value_type));
				memmove(const_cast<value_type*>(p), pBegin, (size_t)(n) * sizeof(value_type));
			}
			else
			{
				pointer pOldEnd = internalLayout().EndPtr();
#if EASTL_EXCEPTIONS_ENABLED
				const size_type nOldSize = internalLayout().GetSize();
#endif
				const value_type* const pMid = pBegin + (nElementsAfter + 1);

				StringAlgorithm::StringUninitializedCopy(pMid, pEnd, internalLayout().EndPtr() + 1);
				internalLayout().SetSize(internalLayout().GetSize() + (n - nElementsAfter));

#if EASTL_EXCEPTIONS_ENABLED
				try
				{
#endif
					// See comment in if block above
					const size_type nSavedSize = internalLayout().GetSize();
					StringAlgorithm::StringUninitializedCopy(p, pOldEnd + 1, internalLayout().EndPtr());
					internalLayout().SetSize(nSavedSize + nElementsAfter);
#if EASTL_EXCEPTIONS_ENABLED
				}
				catch (...)
				{
					internalLayout().SetSize(nOldSize);
					throw;
				}
#endif

				StringAlgorithm::StringUninitializedCopy(pBegin, pMid, const_cast<value_type*>(p));
			}
		}
		else // Else we need to reallocate to implement this.
		{
			const size_type nOldSize = internalLayout().GetSize();
			const size_type nOldCap = capacity();
			size_type nLength;

			if (bCapacityIsSufficient) // If bCapacityIsSufficient is true, then bSourceIsFromSelf must be true.
				nLength = nOldSize + n;
			else
				nLength = GetNewCapacity(nOldCap, (nOldSize + n) - nOldCap);

			pointer pNewBegin = DoAllocate(nLength + 1);

			pointer pNewEnd = StringAlgorithm::StringUninitializedCopy(internalLayout().BeginPtr(), p, pNewBegin);
			pNewEnd = StringAlgorithm::StringUninitializedCopy(pBegin, pEnd, pNewEnd);
			pNewEnd = StringAlgorithm::StringUninitializedCopy(p, internalLayout().EndPtr(), pNewEnd);
			*pNewEnd = 0;

			DeallocateSelf();
			internalLayout().SetHeapBeginPtr(pNewBegin);
			internalLayout().SetHeapCapacity(nLength);
			internalLayout().SetHeapSize(nOldSize + n);
		}
	}

	return internalLayout().BeginPtr() + nPosition;
}

template <typename T, typename Allocator>
typename BasicString<T, Allocator>::iterator
BasicString<T, Allocator>::insert(const_iterator p, std::initializer_list<value_type> ilist)
{
	return insert(p, ilist.begin(), ilist.end());
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator>& BasicString<T, Allocator>::erase(size_type position, size_type n)
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(position > internalLayout().GetSize()))
		ThrowRangeException();
#endif

#if EASTL_ASSERT_ENABLED
	if (EASTL_UNLIKELY(position > internalLayout().GetSize()))
		EASTL_FAIL_MSG("BasicString::erase -- invalid position");
#endif

	erase(internalLayout().BeginPtr() + position,
		  internalLayout().BeginPtr() + position + std::min(n, internalLayout().GetSize() - position));

	return *this;
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::iterator
BasicString<T, Allocator>::erase(const_iterator p)
{
#if EASTL_ASSERT_ENABLED
	if (EASTL_UNLIKELY((p < internalLayout().BeginPtr()) || (p >= internalLayout().EndPtr())))
		EASTL_FAIL_MSG("BasicString::erase -- invalid position");
#endif

	memmove(const_cast<value_type*>(p), p + 1, (size_t)(internalLayout().EndPtr() - p) * sizeof(value_type));
	internalLayout().SetSize(internalLayout().GetSize() - 1);
	return const_cast<value_type*>(p);
}

template <typename T, typename Allocator>
typename BasicString<T, Allocator>::iterator
BasicString<T, Allocator>::erase(const_iterator pBegin, const_iterator pEnd)
{
#if EASTL_ASSERT_ENABLED
	if (EASTL_UNLIKELY((pBegin < internalLayout().BeginPtr()) || (pBegin > internalLayout().EndPtr()) ||
		(pEnd < internalLayout().BeginPtr()) || (pEnd > internalLayout().EndPtr()) || (pEnd < pBegin)))
		EASTL_FAIL_MSG("BasicString::erase -- invalid position");
#endif

	if (pBegin != pEnd)
	{
		memmove(const_cast<value_type*>(pBegin), pEnd, (size_t)((internalLayout().EndPtr() - pEnd) + 1) * sizeof(value_type));
		const size_type n = (size_type)(pEnd - pBegin);
		internalLayout().SetSize(internalLayout().GetSize() - n);
	}
	return const_cast<value_type*>(pBegin);
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::reverse_iterator
BasicString<T, Allocator>::erase(reverse_iterator position)
{
	return reverse_iterator(erase((++position).base()));
}

template <typename T, typename Allocator>
typename BasicString<T, Allocator>::reverse_iterator
BasicString<T, Allocator>::erase(reverse_iterator first, reverse_iterator last)
{
	return reverse_iterator(erase((++last).base(), (++first).base()));
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::replace(size_type position, size_type n, const this_type& x)
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(position > internalLayout().GetSize()))
		ThrowRangeException();
#endif

	const size_type nLength = std::min(n, internalLayout().GetSize() - position);

#if EASTL_STRING_OPT_LENGTH_ERRORS
	if (EASTL_UNLIKELY((internalLayout().GetSize() - nLength) >= (max_size() - x.internalLayout().GetSize())))
		ThrowLengthException();
#endif

	return replace(internalLayout().BeginPtr() + position, internalLayout().BeginPtr() + position + nLength, x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::replace(size_type pos1, size_type n1, const this_type& x, size_type pos2, size_type n2)
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY((pos1 > internalLayout().GetSize()) || (pos2 > x.internalLayout().GetSize())))
		ThrowRangeException();
#endif

	const size_type nLength1 = std::min(n1, internalLayout().GetSize() - pos1);
	const size_type nLength2 = std::min(n2, x.internalLayout().GetSize() - pos2);

#if EASTL_STRING_OPT_LENGTH_ERRORS
	if (EASTL_UNLIKELY((internalLayout().GetSize() - nLength1) >= (max_size() - nLength2)))
		ThrowLengthException();
#endif

	return replace(internalLayout().BeginPtr() + pos1, internalLayout().BeginPtr() + pos1 + nLength1, x.internalLayout().BeginPtr() + pos2, x.internalLayout().BeginPtr() + pos2 + nLength2);
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::replace(size_type position, size_type n1, const value_type* p, size_type n2)
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(position > internalLayout().GetSize()))
		ThrowRangeException();
#endif

	const size_type nLength = std::min(n1, internalLayout().GetSize() - position);

#if EASTL_STRING_OPT_LENGTH_ERRORS
	if (EASTL_UNLIKELY((n2 > max_size()) || ((internalLayout().GetSize() - nLength) >= (max_size() - n2))))
		ThrowLengthException();
#endif

	return replace(internalLayout().BeginPtr() + position, internalLayout().BeginPtr() + position + nLength, p, p + n2);
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::replace(size_type position, size_type n1, const value_type* p)
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(position > internalLayout().GetSize()))
		ThrowRangeException();
#endif

	const size_type nLength = std::min(n1, internalLayout().GetSize() - position);

#if EASTL_STRING_OPT_LENGTH_ERRORS
	const size_type n2 = (size_type)StringAlgorithm::Strlen(p);
	if (EASTL_UNLIKELY((n2 > max_size()) || ((internalLayout().GetSize() - nLength) >= (max_size() - n2))))
		ThrowLengthException();
#endif

	return replace(internalLayout().BeginPtr() + position, internalLayout().BeginPtr() + position + nLength, p, p + StringAlgorithm::Strlen(p));
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::replace(size_type position, size_type n1, size_type n2, value_type c)
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(position > internalLayout().GetSize()))
		ThrowRangeException();
#endif

	const size_type nLength = std::min(n1, internalLayout().GetSize() - position);

#if EASTL_STRING_OPT_LENGTH_ERRORS
	if (EASTL_UNLIKELY((n2 > max_size()) || (internalLayout().GetSize() - nLength) >= (max_size() - n2)))
		ThrowLengthException();
#endif

	return replace(internalLayout().BeginPtr() + position, internalLayout().BeginPtr() + position + nLength, n2, c);
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator>& BasicString<T, Allocator>::replace(const_iterator pBegin, const_iterator pEnd, const this_type& x)
{
	return replace(pBegin, pEnd, x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator>& BasicString<T, Allocator>::replace(const_iterator pBegin, const_iterator pEnd, const value_type* p, size_type n)
{
	return replace(pBegin, pEnd, p, p + n);
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator>& BasicString<T, Allocator>::replace(const_iterator pBegin, const_iterator pEnd, const value_type* p)
{
	return replace(pBegin, pEnd, p, p + StringAlgorithm::Strlen(p));
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::replace(const_iterator pBegin, const_iterator pEnd, size_type n, value_type c)
{
#if EASTL_ASSERT_ENABLED
	if (EASTL_UNLIKELY((pBegin < internalLayout().BeginPtr()) || (pBegin > internalLayout().EndPtr()) ||
		(pEnd < internalLayout().BeginPtr()) || (pEnd > internalLayout().EndPtr()) || (pEnd < pBegin)))
		EASTL_FAIL_MSG("BasicString::replace -- invalid position");
#endif

	const size_type nLength = static_cast<size_type>(pEnd - pBegin);

	if (nLength >= n)
	{
		StringAlgorithm::AssignN(const_cast<value_type*>(pBegin), n, c);
		erase(pBegin + n, pEnd);
	}
	else
	{
		StringAlgorithm::AssignN(const_cast<value_type*>(pBegin), nLength, c);
		insert(pEnd, n - nLength, c);
	}
	return *this;
}

template <typename T, typename Allocator>
BasicString<T, Allocator>& BasicString<T, Allocator>::replace(const_iterator pBegin1, const_iterator pEnd1, const value_type* pBegin2, const value_type* pEnd2)
{
#if EASTL_ASSERT_ENABLED
	if (EASTL_UNLIKELY((pBegin1 < internalLayout().BeginPtr()) || (pBegin1 > internalLayout().EndPtr()) ||
		(pEnd1 < internalLayout().BeginPtr()) || (pEnd1 > internalLayout().EndPtr()) || (pEnd1 < pBegin1)))
		EASTL_FAIL_MSG("BasicString::replace -- invalid position");
#endif

	const size_type nLength1 = (size_type)(pEnd1 - pBegin1);
	const size_type nLength2 = (size_type)(pEnd2 - pBegin2);

	if (nLength1 >= nLength2) // If we have a non-expanding operation...
	{
		if ((pBegin2 > pEnd1) || (pEnd2 <= pBegin1))  // If we have a non-overlapping operation...
			memcpy(const_cast<value_type*>(pBegin1), pBegin2, (size_t)(pEnd2 - pBegin2) * sizeof(value_type));
		else
			memmove(const_cast<value_type*>(pBegin1), pBegin2, (size_t)(pEnd2 - pBegin2) * sizeof(value_type));
		erase(pBegin1 + nLength2, pEnd1);
	}
	else // Else we are expanding.
	{
		if ((pBegin2 > pEnd1) || (pEnd2 <= pBegin1)) // If we have a non-overlapping operation...
		{
			const value_type* const pMid2 = pBegin2 + nLength1;

			if ((pEnd2 <= pBegin1) || (pBegin2 > pEnd1))
				memcpy(const_cast<value_type*>(pBegin1), pBegin2, (size_t)(pMid2 - pBegin2) * sizeof(value_type));
			else
				memmove(const_cast<value_type*>(pBegin1), pBegin2, (size_t)(pMid2 - pBegin2) * sizeof(value_type));
			insert(pEnd1, pMid2, pEnd2);
		}
		else // else we have an overlapping operation.
		{
			// I can't think of any easy way of doing this without allocating temporary memory.
			const size_type nOldSize = internalLayout().GetSize();
			const size_type nOldCap = capacity();
			const size_type nNewCapacity = GetNewCapacity(nOldCap, (nOldSize + (nLength2 - nLength1)) - nOldCap);

			pointer pNewBegin = DoAllocate(nNewCapacity + 1);

			pointer pNewEnd = StringAlgorithm::StringUninitializedCopy(internalLayout().BeginPtr(), pBegin1, pNewBegin);
			pNewEnd = StringAlgorithm::StringUninitializedCopy(pBegin2, pEnd2, pNewEnd);
			pNewEnd = StringAlgorithm::StringUninitializedCopy(pEnd1, internalLayout().EndPtr(), pNewEnd);
			*pNewEnd = 0;

			DeallocateSelf();
			internalLayout().SetHeapBeginPtr(pNewBegin);
			internalLayout().SetHeapCapacity(nNewCapacity);
			internalLayout().SetHeapSize(nOldSize + (nLength2 - nLength1));
		}
	}
	return *this;
}

template <typename T, typename Allocator>
typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::copy(value_type* p, size_type n, size_type position) const
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(position > internalLayout().GetSize()))
		ThrowRangeException();
#endif

	// C++ std says the effects of this function are as if calling char_traits::copy()
	// thus the 'p' must not overlap *this string, so we can use memcpy
	const size_type nLength = std::min(n, internalLayout().GetSize() - position);
	StringAlgorithm::StringUninitializedCopy(internalLayout().BeginPtr() + position, internalLayout().BeginPtr() + position + nLength, p);
	return nLength;
}


template <typename T, typename Allocator>
void BasicString<T, Allocator>::swap(this_type& x)
{
	if ((internalLayout().IsSSO() && x.internalLayout().IsSSO())) // If allocators are equivalent...
	{
		// We leave mAllocator as-is.
		std::swap(internalLayout(), x.internalLayout());
	}
	else // else swap the contents.
	{
		const this_type temp(*this); // Can't call std::swap because that would
		*this = x;                   // itself call this member swap function.
		x = temp;
	}
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find(const this_type& x, size_type position) const noexcept
{
	return find(x.internalLayout().BeginPtr(), position, x.internalLayout().GetSize());
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find(const value_type* p, size_type position) const
{
	return find(p, position, (size_type)StringAlgorithm::Strlen(p));
}

template <typename T, typename Allocator>
typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find(const value_type* p, size_type position, size_type n) const
{
	// It is not clear what the requirements are for position, but since the C++ standard
	// appears to be silent it is assumed for now that position can be any value.
	//#if EASTL_ASSERT_ENABLED
	//    if(EASTL_UNLIKELY(position > (size_type)(mpEnd - mpBegin)))
	//        EASTL_FAIL_MSG("BasicString::find -- invalid position");
	//#endif

	if (((npos - n) >= position) && (position + n) <= internalLayout().GetSize()) // If the range is valid...
	{
		const value_type* const pTemp = std::search(internalLayout().BeginPtr() + position, internalLayout().EndPtr(), p, p + n);

		if ((pTemp != internalLayout().EndPtr()) || (n == 0))
			return (size_type)(pTemp - internalLayout().BeginPtr());
	}
	return npos;
}

template <typename T, typename Allocator>
typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find(value_type c, size_type position) const noexcept
{
	// It is not clear what the requirements are for position, but since the C++ standard
	// appears to be silent it is assumed for now that position can be any value.
	//#if EASTL_ASSERT_ENABLED
	//    if(EASTL_UNLIKELY(position > (size_type)(mpEnd - mpBegin)))
	//        EASTL_FAIL_MSG("BasicString::find -- invalid position");
	//#endif

	if (position < internalLayout().GetSize()) // If the position is valid...
	{
		const const_iterator pResult = std::find(internalLayout().BeginPtr() + position, internalLayout().EndPtr(), c);

		if (pResult != internalLayout().EndPtr())
			return (size_type)(pResult - internalLayout().BeginPtr());
	}
	return npos;
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::rfind(const this_type& x, size_type position) const noexcept
{
	return rfind(x.internalLayout().BeginPtr(), position, x.internalLayout().GetSize());
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::rfind(const value_type* p, size_type position) const
{
	return rfind(p, position, (size_type)StringAlgorithm::Strlen(p));
}

template <typename T, typename Allocator>
typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::rfind(const value_type* p, size_type position, size_type n) const
{
	// Disabled because it's not clear what values are valid for position.
	// It is documented that npos is a valid value, though. We return npos and
	// don't crash if postion is any invalid value.
	//#if EASTL_ASSERT_ENABLED
	//    if(EASTL_UNLIKELY((position != npos) && (position > (size_type)(mpEnd - mpBegin))))
	//        EASTL_FAIL_MSG("BasicString::rfind -- invalid position");
	//#endif

	// Note that a search for a zero length string starting at position = end() returns end() and not npos.
	// Note by Paul Pedriana: I am not sure how this should behave in the case of n == 0 and position > size.
	// The standard seems to suggest that rfind doesn't act exactly the same as find in that input position
	// can be > size and the return value can still be other than npos. Thus, if n == 0 then you can
	// never return npos, unlike the case with find.
	const size_type nLength = internalLayout().GetSize();

	if (n <= nLength)
	{
		if (n)
		{
			const const_iterator pEnd = internalLayout().BeginPtr() + std::min(nLength - n, position) + n;
			const const_iterator pResult = StringAlgorithm::StringRSearch(internalLayout().BeginPtr(), pEnd, p, p + n);

			if (pResult != pEnd)
				return (size_type)(pResult - internalLayout().BeginPtr());
		}
		else
			return std::min(nLength, position);
	}
	return npos;
}

template <typename T, typename Allocator>
typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::rfind(value_type c, size_type position) const noexcept
{
	// If n is zero or position is >= size, we return npos.
	const size_type nLength = internalLayout().GetSize();

	if (nLength)
	{
		const value_type* const pEnd = internalLayout().BeginPtr() + std::min(nLength - 1, position) + 1;
		const value_type* const pResult = StringAlgorithm::StringRFind(pEnd, internalLayout().BeginPtr(), c);

		if (pResult != internalLayout().BeginPtr())
			return (size_type)((pResult - 1) - internalLayout().BeginPtr());
	}
	return npos;
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find_first_of(const this_type& x, size_type position) const noexcept
{
	return find_first_of(x.internalLayout().BeginPtr(), position, x.internalLayout().GetSize());
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find_first_of(const value_type* p, size_type position) const
{
	return find_first_of(p, position, (size_type)StringAlgorithm::Strlen(p));
}

template <typename T, typename Allocator>
typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find_first_of(const value_type* p, size_type position, size_type n) const
{
	// If position is >= size, we return npos.
	if ((position < internalLayout().GetSize()))
	{
		const value_type* const pBegin = internalLayout().BeginPtr() + position;
		const const_iterator pResult = StringAlgorithm::StringFindFirstOf(pBegin, internalLayout().EndPtr(), p, p + n);

		if (pResult != internalLayout().EndPtr())
			return (size_type)(pResult - internalLayout().BeginPtr());
	}
	return npos;
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find_first_of(value_type c, size_type position) const noexcept
{
	return find(c, position);
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find_last_of(const this_type& x, size_type position) const noexcept
{
	return find_last_of(x.internalLayout().BeginPtr(), position, x.internalLayout().GetSize());
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find_last_of(const value_type* p, size_type position) const
{
	return find_last_of(p, position, (size_type)StringAlgorithm::Strlen(p));
}

template <typename T, typename Allocator>
typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find_last_of(const value_type* p, size_type position, size_type n) const
{
	// If n is zero or position is >= size, we return npos.
	const size_type nLength = internalLayout().GetSize();

	if (nLength)
	{
		const value_type* const pEnd = internalLayout().BeginPtr() + std::min(nLength - 1, position) + 1;
		const value_type* const pResult = StringAlgorithm::StringRFindFirstOf(pEnd, internalLayout().BeginPtr(), p, p + n);

		if (pResult != internalLayout().BeginPtr())
			return (size_type)((pResult - 1) - internalLayout().BeginPtr());
	}
	return npos;
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find_last_of(value_type c, size_type position) const noexcept
{
	return rfind(c, position);
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find_first_not_of(const this_type& x, size_type position) const noexcept
{
	return find_first_not_of(x.internalLayout().BeginPtr(), position, x.internalLayout().GetSize());
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find_first_not_of(const value_type* p, size_type position) const
{
	return find_first_not_of(p, position, (size_type)StringAlgorithm::Strlen(p));
}

template <typename T, typename Allocator>
typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find_first_not_of(const value_type* p, size_type position, size_type n) const
{
	if (position <= internalLayout().GetSize())
	{
		const const_iterator pResult =
			StringAlgorithm::StringFindFirstNotOf(internalLayout().BeginPtr() + position, internalLayout().EndPtr(), p, p + n);

		if (pResult != internalLayout().EndPtr())
			return (size_type)(pResult - internalLayout().BeginPtr());
	}
	return npos;
}

template <typename T, typename Allocator>
typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find_first_not_of(value_type c, size_type position) const noexcept
{
	if (position <= internalLayout().GetSize())
	{
		// Todo: Possibly make a specialized version of CharTypeStringFindFirstNotOf(pBegin, pEnd, c).
		const const_iterator pResult =
			StringAlgorithm::StringFindFirstNotOf(internalLayout().BeginPtr() + position, internalLayout().EndPtr(), &c, &c + 1);

		if (pResult != internalLayout().EndPtr())
			return (size_type)(pResult - internalLayout().BeginPtr());
	}
	return npos;
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find_last_not_of(const this_type& x, size_type position) const noexcept
{
	return find_last_not_of(x.internalLayout().BeginPtr(), position, x.internalLayout().GetSize());
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find_last_not_of(const value_type* p, size_type position) const
{
	return find_last_not_of(p, position, (size_type)StringAlgorithm::Strlen(p));
}

template <typename T, typename Allocator>
typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find_last_not_of(const value_type* p, size_type position, size_type n) const
{
	const size_type nLength = internalLayout().GetSize();

	if (nLength)
	{
		const value_type* const pEnd = internalLayout().BeginPtr() + std::min(nLength - 1, position) + 1;
		const value_type* const pResult = StringAlgorithm::StringRFindFirstNotOf(pEnd, internalLayout().BeginPtr(), p, p + n);

		if (pResult != internalLayout().BeginPtr())
			return (size_type)((pResult - 1) - internalLayout().BeginPtr());
	}
	return npos;
}

template <typename T, typename Allocator>
typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::find_last_not_of(value_type c, size_type position) const noexcept
{
	const size_type nLength = internalLayout().GetSize();

	if (nLength)
	{
		// Todo: Possibly make a specialized version of CharTypeStringRFindFirstNotOf(pBegin, pEnd, c).
		const value_type* const pEnd = internalLayout().BeginPtr() + std::min(nLength - 1, position) + 1;
		const value_type* const pResult = StringAlgorithm::StringRFindFirstNotOf(pEnd, internalLayout().BeginPtr(), &c, &c + 1);

		if (pResult != internalLayout().BeginPtr())
			return (size_type)((pResult - 1) - internalLayout().BeginPtr());
	}
	return npos;
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator> BasicString<T, Allocator>::substr(size_type position, size_type n) const
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(position > internalLayout().GetSize()))
		ThrowRangeException();
#elif EASTL_ASSERT_ENABLED
	if (EASTL_UNLIKELY(position > internalLayout().GetSize()))
		EASTL_FAIL_MSG("BasicString::substr -- invalid position");
#endif

	// C++ std says the return string allocator must be default constructed, not a copy of this->get_allocator()
	return BasicString(
		internalLayout().BeginPtr() + position,
		internalLayout().BeginPtr() + position +
			std::min(n, internalLayout().GetSize() - position));
}

template <typename T, typename Allocator>
inline int BasicString<T, Allocator>::compare(const this_type& x) const noexcept
{
	return compare(internalLayout().BeginPtr(), internalLayout().EndPtr(), x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
}

template <typename T, typename Allocator>
inline int BasicString<T, Allocator>::compare(size_type pos1, size_type n1, const this_type& x) const
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(pos1 > internalLayout().GetSize()))
		ThrowRangeException();
#endif

	return compare(
		internalLayout().BeginPtr() + pos1,
		internalLayout().BeginPtr() + pos1 + std::min(n1, internalLayout().GetSize() - pos1),
		x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
}

template <typename T, typename Allocator>
inline int BasicString<T, Allocator>::compare(size_type pos1, size_type n1, const this_type& x, size_type pos2, size_type n2) const
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY((pos1 > (size_type)(internalLayout().EndPtr() - internalLayout().BeginPtr())) ||
		(pos2 > (size_type)(x.internalLayout().EndPtr() - x.internalLayout().BeginPtr()))))
		ThrowRangeException();
#endif

	return compare(internalLayout().BeginPtr() + pos1,
				   internalLayout().BeginPtr() + pos1 + std::min(n1, internalLayout().GetSize() - pos1),
				   x.internalLayout().BeginPtr() + pos2,
				   x.internalLayout().BeginPtr() + pos2 + std::min(n2, x.internalLayout().GetSize() - pos2));
}

template <typename T, typename Allocator>
inline int BasicString<T, Allocator>::compare(const value_type* p) const
{
	return compare(internalLayout().BeginPtr(), internalLayout().EndPtr(), p, p + StringAlgorithm::Strlen(p));
}

template <typename T, typename Allocator>
inline int BasicString<T, Allocator>::compare(size_type pos1, size_type n1, const value_type* p) const
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(pos1 > internalLayout().GetSize()))
		ThrowRangeException();
#endif

	return compare(internalLayout().BeginPtr() + pos1,
				   internalLayout().BeginPtr() + pos1 + std::min(n1, internalLayout().GetSize() - pos1),
				   p,
				   p + StringAlgorithm::Strlen(p));
}

template <typename T, typename Allocator>
inline int BasicString<T, Allocator>::compare(size_type pos1, size_type n1, const value_type* p, size_type n2) const
{
#if EASTL_STRING_OPT_RANGE_ERRORS
	if (EASTL_UNLIKELY(pos1 > internalLayout().GetSize()))
		ThrowRangeException();
#endif

	return compare(internalLayout().BeginPtr() + pos1,
				   internalLayout().BeginPtr() + pos1 + std::min(n1, internalLayout().GetSize() - pos1),
				   p,
				   p + n2);
}

// make_lower
// This is a very simple ASCII-only case conversion function
// Anything more complicated should use a more powerful separate library.
template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::make_lower()
{
	for (pointer p = internalLayout().BeginPtr(); p < internalLayout().EndPtr(); ++p)
		*p = (value_type)std::tolower(*p);
}

// make_upper
// This is a very simple ASCII-only case conversion function
// Anything more complicated should use a more powerful separate library.
template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::make_upper()
{
	for (pointer p = internalLayout().BeginPtr(); p < internalLayout().EndPtr(); ++p)
		*p = (value_type)std::toupper(*p);
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::ltrim()
{
	const value_type array[] = { ' ', '\t', 0 }; // This is a pretty simplistic view of whitespace.
	erase(0, find_first_not_of(array));
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::rtrim()
{
	const value_type array[] = { ' ', '\t', 0 }; // This is a pretty simplistic view of whitespace.
	erase(find_last_not_of(array) + 1);
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::trim()
{
	ltrim();
	rtrim();
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::ltrim(const value_type* p)
{
	erase(0, find_first_not_of(p));
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::rtrim(const value_type* p)
{
	erase(find_last_not_of(p) + 1);
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::trim(const value_type* p)
{
	ltrim(p);
	rtrim(p);
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator> BasicString<T, Allocator>::left(size_type n) const
{
	const size_type nLength = length();
	if (n < nLength)
		return substr(0, n);
	// C++ std says that substr must return default constructed allocated, but we do not.
	// Instead it is much more practical to provide the copy of the current allocator
	return BasicString(*this);
}

template <typename T, typename Allocator>
inline BasicString<T, Allocator> BasicString<T, Allocator>::right(size_type n) const
{
	const size_type nLength = length();
	if (n < nLength)
		return substr(nLength - n, n);
	// C++ std says that substr must return default constructed allocated, but we do not.
	// Instead it is much more practical to provide the copy of the current allocator
	return BasicString(*this);
}

template <typename T, typename Allocator>
int BasicString<T, Allocator>::compare(const value_type* pBegin1, const value_type* pEnd1,
										const value_type* pBegin2, const value_type* pEnd2)
{
	const difference_type n1 = pEnd1 - pBegin1;
	const difference_type n2 = pEnd2 - pBegin2;
	const difference_type nMin = std::min(n1, n2);
	const int       cmp = StringAlgorithm::Compare(pBegin1, pBegin2, (size_t)nMin);

	return (cmp != 0 ? cmp : (n1 < n2 ? -1 : (n1 > n2 ? 1 : 0)));
}

template <typename T, typename Allocator>
int BasicString<T, Allocator>::comparei(const value_type* pBegin1, const value_type* pEnd1,
										 const value_type* pBegin2, const value_type* pEnd2)
{
	const difference_type n1 = pEnd1 - pBegin1;
	const difference_type n2 = pEnd2 - pBegin2;
	const difference_type nMin = std::min(n1, n2);
	const int       cmp = StringAlgorithm::CompareI(pBegin1, pBegin2, (size_t)nMin);

	return (cmp != 0 ? cmp : (n1 < n2 ? -1 : (n1 > n2 ? 1 : 0)));
}

template <typename T, typename Allocator>
inline int BasicString<T, Allocator>::comparei(const this_type& x) const noexcept
{
	return comparei(internalLayout().BeginPtr(), internalLayout().EndPtr(), x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
}

template <typename T, typename Allocator>
inline int BasicString<T, Allocator>::comparei(const value_type* p) const
{
	return comparei(internalLayout().BeginPtr(), internalLayout().EndPtr(), p, p + StringAlgorithm::Strlen(p));
}

template <typename T, typename Allocator>
typename BasicString<T, Allocator>::iterator
BasicString<T, Allocator>::InsertInternal(const_iterator p, value_type c)
{
	iterator pNewPosition = const_cast<value_type*>(p);

	if ((internalLayout().EndPtr() + 1) <= internalLayout().CapacityPtr())
	{
		const size_type nSavedSize = internalLayout().GetSize();
		memmove(const_cast<value_type*>(p) + 1, p, (size_t)(internalLayout().EndPtr() - p) * sizeof(value_type));
		*(internalLayout().EndPtr() + 1) = 0;
		*pNewPosition = c;
		internalLayout().SetSize(nSavedSize + 1);
	}
	else
	{
		const size_type nOldSize = internalLayout().GetSize();
		const size_type nOldCap = capacity();
		const size_type nLength = GetNewCapacity(nOldCap, 1);

		iterator pNewBegin = DoAllocate(nLength + 1);

		pNewPosition = StringAlgorithm::StringUninitializedCopy(internalLayout().BeginPtr(), p, pNewBegin);
		*pNewPosition = c;

		iterator pNewEnd = pNewPosition + 1;
		pNewEnd = StringAlgorithm::StringUninitializedCopy(p, internalLayout().EndPtr(), pNewEnd);
		*pNewEnd = 0;

		DeallocateSelf();
		internalLayout().SetHeapBeginPtr(pNewBegin);
		internalLayout().SetHeapCapacity(nLength);
		internalLayout().SetHeapSize(nOldSize + 1);
	}
	return pNewPosition;
}

template <typename T, typename Allocator>
void BasicString<T, Allocator>::SizeInitialize(size_type n, value_type c)
{
	AllocateSelf(n);

	StringAlgorithm::StringUninitializedFillN(internalLayout().BeginPtr(), n, c);
	*internalLayout().EndPtr() = 0;
}

template <typename T, typename Allocator>
void BasicString<T, Allocator>::RangeInitialize(const value_type* pBegin, const value_type* pEnd)
{
#if EASTL_STRING_OPT_ARGUMENT_ERRORS
	if (EASTL_UNLIKELY(!pBegin && (pEnd < pBegin))) // 21.4.2 p7
		ThrowInvalidArgumentException();
#endif

	const size_type n = (size_type)(pEnd - pBegin);

	AllocateSelf(n);

	StringAlgorithm::StringUninitializedCopy(pBegin, pEnd, internalLayout().BeginPtr());
	*internalLayout().EndPtr() = 0;
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::RangeInitialize(const value_type* pBegin)
{
#if EASTL_STRING_OPT_ARGUMENT_ERRORS
	if (EASTL_UNLIKELY(!pBegin))
		ThrowInvalidArgumentException();
#endif

	RangeInitialize(pBegin, pBegin + StringAlgorithm::Strlen(pBegin));
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::value_type*
BasicString<T, Allocator>::DoAllocate(size_type n)
{
	return (value_type*)get_allocator().Allocate(n * sizeof(value_type), alignof(T));
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::DoFree(value_type* p, size_type n)
{
	if (p)
	{
		get_allocator().Free(p);
	}
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::GetNewCapacity(size_type currentCapacity)
{
	return GetNewCapacity(currentCapacity, 1);
}

template <typename T, typename Allocator>
inline typename BasicString<T, Allocator>::size_type
BasicString<T, Allocator>::GetNewCapacity(size_type currentCapacity, size_type minimumGrowSize)
{
#if EASTL_STRING_OPT_LENGTH_ERRORS
	const size_type nRemainingSize = max_size() - currentCapacity;
	if (EASTL_UNLIKELY((minimumGrowSize > nRemainingSize)))
	{
		ThrowLengthException();
	}
#endif

	const size_type nNewCapacity = std::max(currentCapacity + minimumGrowSize, currentCapacity * 2);

	return nNewCapacity;
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::AllocateSelf()
{
	internalLayout().ResetToSSO();
}

template <typename T, typename Allocator>
void BasicString<T, Allocator>::AllocateSelf(size_type n)
{
#if EASTL_ASSERT_ENABLED
	if (EASTL_UNLIKELY(n >= 0x40000000))
		EASTL_FAIL_MSG("BasicString::AllocateSelf -- improbably large request.");
#endif

#if EASTL_STRING_OPT_LENGTH_ERRORS
	if (EASTL_UNLIKELY(n > max_size()))
		ThrowLengthException();
#endif

	if (n > SSOLayout::SSO_CAPACITY)
	{
		pointer pBegin = DoAllocate(n + 1);
		internalLayout().SetHeapBeginPtr(pBegin);
		internalLayout().SetHeapCapacity(n);
		internalLayout().SetHeapSize(n);
	}
	else
		internalLayout().SetSSOSize(n);
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::DeallocateSelf()
{
	if (internalLayout().IsHeap())
	{
		DoFree(internalLayout().BeginPtr(), internalLayout().GetHeapCapacity() + 1);
	}
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::ThrowLengthException() const
{
#if EASTL_EXCEPTIONS_ENABLED
	throw std::length_error("BasicString -- length_error");
#elif EASTL_ASSERT_ENABLED
	EASTL_FAIL_MSG("BasicString -- length_error");
#endif
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::ThrowRangeException() const
{
#if EASTL_EXCEPTIONS_ENABLED
	throw std::out_of_range("BasicString -- out of range");
#elif EASTL_ASSERT_ENABLED
	EASTL_FAIL_MSG("BasicString -- out of range");
#endif
}

template <typename T, typename Allocator>
inline void BasicString<T, Allocator>::ThrowInvalidArgumentException() const
{
#if EASTL_EXCEPTIONS_ENABLED
	throw std::invalid_argument("BasicString -- invalid argument");
#elif EASTL_ASSERT_ENABLED
	EASTL_FAIL_MSG("BasicString -- invalid argument");
#endif
}

// iterator operators
template <typename T, typename Allocator>
inline bool operator==(const typename BasicString<T, Allocator>::reverse_iterator& r1,
					   const typename BasicString<T, Allocator>::reverse_iterator& r2)
{
	return r1.current == r2.current;
}

template <typename T, typename Allocator>
inline bool operator!=(const typename BasicString<T, Allocator>::reverse_iterator& r1,
					   const typename BasicString<T, Allocator>::reverse_iterator& r2)
{
	return r1.current != r2.current;
}

// Operator +
template <typename T, typename Allocator>
BasicString<T, Allocator> operator+(const BasicString<T, Allocator>& a, const BasicString<T, Allocator>& b)
{
	typedef typename BasicString<T, Allocator>::CtorDoNotInitialize CtorDoNotInitialize;
	CtorDoNotInitialize cDNI; // GCC 2.x forces us to declare a named temporary like this.
	BasicString<T, Allocator> result(cDNI, a.size() + b.size(), const_cast<BasicString<T, Allocator>&>(a).get_allocator()); // Note that we choose to assign a's allocator.
	result.append(a);
	result.append(b);
	return result;
}

template <typename T, typename Allocator>
BasicString<T, Allocator> operator+(const typename BasicString<T, Allocator>::value_type* p, const BasicString<T, Allocator>& b)
{
	typedef typename BasicString<T, Allocator>::CtorDoNotInitialize CtorDoNotInitialize;
	CtorDoNotInitialize cDNI; // GCC 2.x forces us to declare a named temporary like this.
	const typename BasicString<T, Allocator>::size_type n = (typename BasicString<T, Allocator>::size_type)StringAlgorithm::Strlen(p);
	BasicString<T, Allocator> result(cDNI, n + b.size(), const_cast<BasicString<T, Allocator>&>(b).get_allocator());
	result.append(p, p + n);
	result.append(b);
	return result;
}

template <typename T, typename Allocator>
BasicString<T, Allocator> operator+(typename BasicString<T, Allocator>::value_type c, const BasicString<T, Allocator>& b)
{
	typedef typename BasicString<T, Allocator>::CtorDoNotInitialize CtorDoNotInitialize;
	CtorDoNotInitialize cDNI; // GCC 2.x forces us to declare a named temporary like this.
	BasicString<T, Allocator> result(cDNI, 1 + b.size(), const_cast<BasicString<T, Allocator>&>(b).get_allocator());
	result.push_back(c);
	result.append(b);
	return result;
}

template <typename T, typename Allocator>
BasicString<T, Allocator> operator+(const BasicString<T, Allocator>& a, const typename BasicString<T, Allocator>::value_type* p)
{
	typedef typename BasicString<T, Allocator>::CtorDoNotInitialize CtorDoNotInitialize;
	CtorDoNotInitialize cDNI; // GCC 2.x forces us to declare a named temporary like this.
	const typename BasicString<T, Allocator>::size_type n = (typename BasicString<T, Allocator>::size_type)StringAlgorithm::Strlen(p);
	BasicString<T, Allocator> result(cDNI, a.size() + n, const_cast<BasicString<T, Allocator>&>(a).get_allocator());
	result.append(a);
	result.append(p, p + n);
	return result;
}

template <typename T, typename Allocator>
BasicString<T, Allocator> operator+(const BasicString<T, Allocator>& a, typename BasicString<T, Allocator>::value_type c)
{
	typedef typename BasicString<T, Allocator>::CtorDoNotInitialize CtorDoNotInitialize;
	CtorDoNotInitialize cDNI; // GCC 2.x forces us to declare a named temporary like this.
	BasicString<T, Allocator> result(cDNI, a.size() + 1, const_cast<BasicString<T, Allocator>&>(a).get_allocator());
	result.append(a);
	result.push_back(c);
	return result;
}

template <typename T, typename Allocator>
BasicString<T, Allocator> operator+(BasicString<T, Allocator>&& a, BasicString<T, Allocator>&& b)
{
	a.append(b); // Using an rvalue by name results in it becoming an lvalue.
	return std::move(a);
}

template <typename T, typename Allocator>
BasicString<T, Allocator> operator+(BasicString<T, Allocator>&& a, const BasicString<T, Allocator>& b)
{
	a.append(b);
	return std::move(a);
}

template <typename T, typename Allocator>
BasicString<T, Allocator> operator+(const typename BasicString<T, Allocator>::value_type* p, BasicString<T, Allocator>&& b)
{
	b.insert(0, p);
	return std::move(b);
}

template <typename T, typename Allocator>
BasicString<T, Allocator> operator+(BasicString<T, Allocator>&& a, const typename BasicString<T, Allocator>::value_type* p)
{
	a.append(p);
	return std::move(a);
}

template <typename T, typename Allocator>
BasicString<T, Allocator> operator+(BasicString<T, Allocator>&& a, typename BasicString<T, Allocator>::value_type c)
{
	a.push_back(c);
	return std::move(a);
}

template <typename T, typename Allocator>
inline bool BasicString<T, Allocator>::validate() const noexcept
{
	if ((internalLayout().BeginPtr() == nullptr) || (internalLayout().EndPtr() == nullptr))
		return false;
	if (internalLayout().EndPtr() < internalLayout().BeginPtr())
		return false;
	if (internalLayout().CapacityPtr() < internalLayout().EndPtr())
		return false;
	if (*internalLayout().EndPtr() != 0)
		return false;
	return true;
}

// Operator== and operator!=
template <typename T, typename Allocator>
inline bool operator==(const BasicString<T, Allocator>& a, const BasicString<T, Allocator>& b)
{
	return ((a.size() == b.size()) && (StringAlgorithm::Compare(a.data(), b.data(), (size_t)a.size()) == 0));
}

template <typename T, typename Allocator>
inline bool operator==(const typename BasicString<T, Allocator>::value_type* p, const BasicString<T, Allocator>& b)
{
	typedef typename BasicString<T, Allocator>::size_type string_size_type;
	const string_size_type n = (string_size_type)StringAlgorithm::Strlen(p);
	return ((n == b.size()) && (StringAlgorithm::Compare(p, b.data(), (size_t)n) == 0));
}

template <typename T, typename Allocator>
inline bool operator==(const BasicString<T, Allocator>& a, const typename BasicString<T, Allocator>::value_type* p)
{
	typedef typename BasicString<T, Allocator>::size_type string_size_type;
	const string_size_type n = (string_size_type)StringAlgorithm::Strlen(p);
	return ((a.size() == n) && (StringAlgorithm::Compare(a.data(), p, (size_t)n) == 0));
}

template <typename T, typename Allocator>
inline auto operator<=>(const BasicString<T, Allocator>& a, const BasicString<T, Allocator>& b)
{
	return BasicString<T, Allocator>::compare(a.begin(), a.end(), b.begin(), b.end()) <=> 0;
}

template <typename T, typename Allocator>
inline auto operator<=>(const BasicString<T, Allocator>& a, const typename BasicString<T, Allocator>::value_type* p)
{
	typedef typename BasicString<T, Allocator>::size_type string_size_type;
	const string_size_type n = (string_size_type)StringAlgorithm::Strlen(p);
	return BasicString<T, Allocator>::compare(a.begin(), a.end(), p, p + n) <=> 0;
}

template <typename T, typename Allocator>
inline auto operator<=>(const BasicString<T, Allocator>& a, const typename BasicString<T, Allocator>::view_type v)
{
	typedef typename BasicString<T, Allocator>::view_type view_type;
	return static_cast<view_type>(a) <=> v;
}

template<typename T, typename Allocator>
size_t SplitStringWithDelimiter(const BasicString<T, Allocator>& inString, BasicString<T, Allocator>& outString, size_t offset, const T delimiter = ' ')
{
	if (offset >= inString.size())
	{
		outString.clear();
		return BasicString<T, Allocator>::npos;
	}

	const size_t pos = inString.find(delimiter, offset);

	if (pos	!= BasicString<T, Allocator>::npos)
	{
		outString = inString.substr(offset, (pos - offset));
	}
	else
	{
		outString = inString.substr(offset);
	}

	return (pos != BasicString<T, Allocator>::npos) ? pos + 1 : pos;
}

using String = BasicString<char>;
using WString = BasicString<wchar_t>;

using String8 = BasicString<char>;
using String16 = BasicString<char16_t>;
using String32 = BasicString<char32_t>;

using U8String = BasicString<char8_t>;
using U16String = BasicString<char16_t>;
using U32String = BasicString<char32_t>;

VTCOREUTIL_API unsigned long long StoUll(const String& str, size_t* index = nullptr, int32_t base = 10);
VTCOREUTIL_API int StoI(const String& str, size_t* index = nullptr, int32_t base = 10);
VTCOREUTIL_API float StoF(const String& str, size_t* index = nullptr);

namespace std
{
	template<typename T> struct hash;

	template<>
	struct hash<String>
	{
		size_t operator()(const String& x) const
		{
			const unsigned char* p = (const unsigned char*)x.c_str();
			uint32_t c, result = 2166136261u;
			while ((c = *p++) != 0)
			{
				result = (result * 16777619) ^ c;
			}

			return static_cast<size_t>(result);
		}
	};

	template<>
	struct hash<U8String>
	{
		size_t operator()(const U8String& x) const
		{
			const char8_t* p = (const char8_t*)x.c_str();
			uint32_t c, result = 2166136261u;
			while ((c = *p++) != 0)
			{
				result = (result * 16777619) ^ c;
			}

			return static_cast<size_t>(result);
		}
	};

	template<>
	struct hash<String16>
	{
		size_t operator()(const String16& x) const
		{
			const char16_t* p = (const char16_t*)x.c_str();
			uint32_t c, result = 2166136261u;
			while ((c = *p++) != 0)
			{
				result = (result * 16777619) ^ c;
			}

			return static_cast<size_t>(result);
		}
	};

	template<>
	struct hash<String32>
	{
		size_t operator()(const String32& x) const
		{
			const char16_t* p = (const char16_t*)x.c_str();
			uint32_t c, result = 2166136261u;
			while ((c = *p++) != 0)
			{
				result = (result * 16777619) ^ c;
			}

			return static_cast<size_t>(result);
		}
	};

	template<>
	struct hash<WString>
	{
		size_t operator()(const WString& x) const
		{
			const wchar_t* p = x.c_str();
			uint32_t c, result = 2166136261u;
			while ((c = *p++) != 0)
			{
				result = (result * 16777619) ^ c;
			}

			return static_cast<size_t>(result);
		}
	};

	template<typename CharT, typename Allocator>
	struct formatter<BasicString<char, Allocator>, CharT>
	{
		formatter<basic_string_view<CharT>, CharT> underlying;

		constexpr auto parse(basic_format_parse_context<CharT>& ctx)
		{
			return underlying.parse(ctx);
		}

		template<typename FormatContext>
		auto format(const BasicString<char, Allocator>& str, FormatContext& ctx) const
		{
			if constexpr (std::is_same_v<CharT, char>)
			{
				return underlying.format(
					std::basic_string_view<char>(str.c_str(), str.size()),
					ctx
				);
			}
			else
			{
				const WString tempStr(WString::CtorConvert(), str.c_str(), str.size());

				return underlying.format(
					std::basic_string_view<wchar_t>(tempStr.c_str(), tempStr.size()),
					ctx
				);
			}
		}
	};

	template<typename CharT, typename Allocator>
	struct formatter<BasicString<wchar_t, Allocator>, CharT>
	{
		formatter<basic_string_view<CharT>, CharT> underlying;

		constexpr auto parse(basic_format_parse_context<CharT>& ctx)
		{
			return underlying.parse(ctx);
		}

		template<typename FormatContext>
		auto format(const BasicString<wchar_t, Allocator>& str, FormatContext& ctx) const
		{
			if constexpr (std::is_same_v<CharT, char>)
			{
				const String tempStr(String::CtorConvert(), str.c_str(), str.size());

				return underlying.format(
					std::basic_string_view<char>(tempStr.c_str(), tempStr.size()),
					ctx
				);
			}
			else
			{
				return underlying.format(
					std::basic_string_view<wchar_t>(str.c_str(), str.size()),
					ctx
				);
			}
		}
	};

	template<typename CharT>
	struct formatter<BasicStringView<char>, CharT>
	{
		formatter<basic_string_view<CharT>, CharT> underlying;

		constexpr auto parse(basic_format_parse_context<CharT>& ctx)
		{
			return underlying.parse(ctx);
		}

		template<typename FormatContext>
		auto format(const BasicStringView<char> str, FormatContext& ctx) const
		{
			if constexpr (std::is_same_v<CharT, char>)
			{
				return underlying.format(
					std::basic_string_view<char>(str.data(), str.size()),
					ctx
				);
			}
			else
			{
				const WString tempStr(WString::CtorConvert(), str.data(), str.size());

				return underlying.format(
					std::basic_string_view<wchar_t>(tempStr.c_str(), tempStr.size()),
					ctx
				);
			}
		}
	};

	template<typename CharT>
	struct formatter<BasicStringView<wchar_t>, CharT>
	{
		formatter<basic_string_view<CharT>, CharT> underlying;

		constexpr auto parse(basic_format_parse_context<CharT>& ctx)
		{
			return underlying.parse(ctx);
		}

		template<typename FormatContext>
		auto format(const BasicStringView<wchar_t> str, FormatContext& ctx) const
		{
			if constexpr (std::is_same_v<CharT, char>)
			{
				const String tempStr(String::CtorConvert(), str.data(), str.size());

				return underlying.format(
					std::basic_string_view<char>(tempStr.c_str(), tempStr.size()),
					ctx
				);
			}
			else
			{
				return underlying.format(
					std::basic_string_view<wchar_t>(str.data(), str.size()),
					ctx
				);
			}
		}
	};
}
