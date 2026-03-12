#pragma once

#include "CoreUtilities/CompilerTraits.h"
#include "CoreUtilities/TypeTraits.h"
#include "CoreUtilities/Memory.h"
#include "CoreUtilities/VoltAssert.h"
#include "CoreUtilities/CompressedPair.h"

#include "CoreUtilities/Allocators/ContainerAllocatorTraits.h"
#include "CoreUtilities/Allocators/ContainerAllocators.h"

#include <initializer_list>
#include <iterator>

template<typename T, typename Allocator>
struct VectorBase
{
	using allocator_type = Allocator::template ForElementType<T>;
	using allocator_traits = ContainerAllocatorTraits<Allocator>;

	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	VectorBase();
	VectorBase(const allocator_type& allocator);
	VectorBase(size_type num, const allocator_type& allocator);
	~VectorBase();

	const allocator_type& get_allocator() const noexcept;
	allocator_type& get_allocator() noexcept;
	void set_allocator(const allocator_type& allocator);

protected:
	T*& InternalCapacityPtr() noexcept { return m_capacityAllocator.First(); }
	T* const& InternalCapacityPtr() const noexcept { return m_capacityAllocator.First(); }
	allocator_type& InternalAllocator() noexcept { return m_capacityAllocator.Second(); }
	const allocator_type& InternalAllocator() const noexcept { return m_capacityAllocator.Second(); }

	T* Allocate(size_type num);
	void DoFree(T* ptr);

	T* m_ptrBegin;
	T* m_ptrEnd;
	CompressedPair<T*, allocator_type> m_capacityAllocator;
};

template<typename T, typename AllocatorType = DefaultHeapAllocator>
class Vector : public VectorBase<T, AllocatorType>
{
private:
	typedef VectorBase<T, AllocatorType> base_type;
	typedef VectorBase<T, AllocatorType>::allocator_type allocator_type;

protected:
	using base_type::m_ptrBegin;
	using base_type::m_ptrEnd;
	using base_type::m_capacityAllocator;
	using base_type::Allocate;
	using base_type::DoFree;
	using base_type::InternalCapacityPtr;
	using base_type::InternalAllocator;

public:
	typedef T value_type;
	typedef T* iterator;
	typedef const T* const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	typedef typename base_type::size_type                 size_type;
	typedef typename base_type::difference_type           difference_type;
	typedef typename base_type::allocator_type            allocator_type;
	typedef typename base_type::allocator_traits		  allocator_traits;

	inline static constexpr size_type npos = (size_type)-1;

	constexpr Vector() noexcept = default;
	constexpr Vector(size_type count) noexcept;
	constexpr Vector(size_type count, const T& value) noexcept;
	constexpr Vector(const Vector<T, AllocatorType>& other) noexcept;
	constexpr Vector(Vector<T, AllocatorType>&& other) noexcept;
	constexpr Vector(std::initializer_list<T> initList) noexcept;
	template<typename InputIterator> constexpr Vector(InputIterator begin, InputIterator end) noexcept;

	constexpr ~Vector();

	constexpr Vector<T, AllocatorType>& operator=(const Vector<T, AllocatorType>& other) noexcept;
	constexpr Vector<T, AllocatorType>& operator=(std::initializer_list<T> initList) noexcept;
	constexpr Vector<T, AllocatorType>& operator=(Vector<T, AllocatorType>&& other) noexcept;

	constexpr void swap(Vector<T, AllocatorType>& other);

	constexpr void assign(size_type count, const value_type& value);

	template<typename InputIterator>
	constexpr void assign(InputIterator first, InputIterator last);

	constexpr void assign(std::initializer_list<value_type> initList);

	constexpr iterator append(const Vector<T, AllocatorType>& other) noexcept;

	template<typename InputIterator>
	constexpr iterator append(InputIterator first, InputIterator last) noexcept;

	VT_NODISCARD constexpr iterator begin() noexcept;
	VT_NODISCARD constexpr const_iterator begin() const noexcept;
	VT_NODISCARD constexpr const_iterator cbegin() const noexcept;

	VT_NODISCARD constexpr iterator end() noexcept;
	VT_NODISCARD constexpr const_iterator end() const noexcept;
	VT_NODISCARD constexpr const_iterator cend() const noexcept;

	VT_NODISCARD constexpr reverse_iterator rbegin() noexcept;
	VT_NODISCARD constexpr const_reverse_iterator rbegin() const noexcept;
	VT_NODISCARD constexpr const_reverse_iterator crbegin() const noexcept;

	VT_NODISCARD constexpr reverse_iterator rend() noexcept;
	VT_NODISCARD constexpr const_reverse_iterator rend() const noexcept;
	VT_NODISCARD constexpr const_reverse_iterator crend() const noexcept;

	VT_NODISCARD constexpr bool empty() const noexcept;
	VT_NODISCARD constexpr size_type size() const noexcept;
	VT_NODISCARD constexpr size_type byte_size() const noexcept;
	VT_NODISCARD constexpr size_type capacity() const noexcept;

	constexpr void resize(size_type count, const value_type& value);
	constexpr void resize(size_type count);
	constexpr void resize_uninitialized(size_type count);
	constexpr void reserve(size_type count);
	constexpr void set_capacity(size_type count = npos);
	constexpr void shrink_to_fit();

	VT_NODISCARD constexpr value_type* data() noexcept;
	VT_NODISCARD constexpr const value_type* data() const noexcept;

	VT_NODISCARD constexpr value_type& operator[](size_type position);
	VT_NODISCARD constexpr const value_type& operator[](size_type position) const;

	VT_NODISCARD constexpr value_type& at(size_type position);
	VT_NODISCARD constexpr const value_type& at(size_type position) const;

	VT_NODISCARD constexpr value_type& front();
	VT_NODISCARD constexpr const value_type& front() const;

	VT_NODISCARD constexpr value_type& back();
	VT_NODISCARD constexpr const value_type& back() const;

	constexpr void push_back(const value_type& value);
	constexpr value_type& push_back();
	constexpr void push_back(value_type&& value);
	constexpr void pop_back();

	template<typename... Args>
	constexpr iterator emplace(const_iterator position, Args&&... args);

	template<typename... Args>
	constexpr value_type& emplace_back(Args&&... args);

	constexpr iterator insert(const_iterator position, const value_type& value);
	constexpr iterator insert(const_iterator position, size_type count, const value_type& value);
	constexpr iterator insert(const_iterator position, value_type&& value);
	constexpr iterator insert(const_iterator position, std::initializer_list<value_type> initList);

	template<typename InputIterator>
	constexpr iterator insert(const_iterator position, InputIterator first, InputIterator last);

	constexpr iterator erase_first(const T& value);
	constexpr iterator erase_first_unsorted(const T& value);
	constexpr reverse_iterator erase_last(const T& value);
	constexpr reverse_iterator erase_last_unsorted(const T& value);

	constexpr iterator erase(const_iterator position);
	constexpr iterator erase(const_iterator first, const_iterator last);
	constexpr iterator erase_unsorted(const_iterator position);

	constexpr reverse_iterator erase(const_reverse_iterator position);
	constexpr reverse_iterator erase(const_reverse_iterator first, const_reverse_iterator last);
	constexpr reverse_iterator erase_unsorted(const_reverse_iterator position);

	template<typename PredicateFunctor>
	constexpr void erase_with_predicate(PredicateFunctor&& functor);

	constexpr bool contains(const T& value) const;
	
	template<typename PredicateFunctor>
	constexpr bool contains_with_predicate(PredicateFunctor&& functor) const;

	constexpr iterator find(const T& value);
	constexpr const_iterator find(const T& value) const;

	template<typename PredicateFunctor>
	constexpr iterator find_with_predicate(PredicateFunctor&& functor);

	template<typename PredicateFunctor>
	constexpr const_iterator find_with_predicate(PredicateFunctor&& functor) const;

	constexpr void clear() noexcept;

protected:
	template<bool move> struct ShouldMoveOrCopyTag {};
	using ShouldCopyTag = ShouldMoveOrCopyTag<false>;
	using ShouldMoveTag = ShouldMoveOrCopyTag<true>;

	template<typename ForwardIterator>
	value_type* Reallocate(size_type count, ForwardIterator first, ForwardIterator last, ShouldCopyTag);

	template<typename ForwardIterator>
	value_type* Reallocate(size_type count, ForwardIterator first, ForwardIterator last, ShouldMoveTag);

	template<typename Integer>
	void Initialize(Integer count, Integer value, std::true_type);

	template<typename InputIterator>
	void Initialize(InputIterator begin, InputIterator end, std::false_type);

	template<typename InputIterator>
	void InitializeFromIterator(InputIterator begin, InputIterator end, std::input_iterator_tag);

	template<typename ForwardIterator>
	void InitializeFromIterator(ForwardIterator begin, ForwardIterator end, std::forward_iterator_tag);

	template<typename Integer, bool move>
	void assign(Integer n, Integer value, std::true_type);

	template<typename InputIterator, bool move>
	void assign(InputIterator begin, InputIterator end, std::false_type);

	void AssignValues(size_type count, const T& value);

	template <typename InputIterator, bool move>
	void AssignFromIterator(InputIterator begin, InputIterator end, std::input_iterator_tag);

	template <typename RandomAccessIterator, bool move>
	void AssignFromIterator(RandomAccessIterator begin, RandomAccessIterator end, std::random_access_iterator_tag);

	template <typename Integer>
	void insert(const_iterator position, Integer count, Integer value, std::true_type);

	template <typename InputIterator>
	void insert(const_iterator position, InputIterator first, InputIterator last, std::false_type);

	template <typename InputIterator>
	void InsertFromIterator(const_iterator position, InputIterator first, InputIterator last, std::input_iterator_tag);

	template <typename BidirectionalIterator>
	void InsertFromIterator(const_iterator position, BidirectionalIterator first, BidirectionalIterator last, std::bidirectional_iterator_tag);

	void InsertValues(const_iterator position, size_type count, const value_type& value);

	template<typename... Args>
	void InsertValue(const_iterator position, Args&&... args);

	template<typename... Args>
	void InsertValueAtEnd(Args&&... args);

	void InsertValuesAtEnd(size_type count); // Default constructs count values
	void InsertValuesAtEnd(size_type count, const value_type& value);

	void ClearCapacity();

	size_type GetNewCapacity(size_type currentCapacity);
	void Grow(size_type count);

	void InitializeAllocation(size_type count);
};

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::Vector(size_type count) noexcept
{
	InitializeAllocation(count);
	UninitializedValueConstructCount(m_ptrBegin, count);
	m_ptrEnd = m_ptrBegin + count;
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::Vector(size_type count, const T& value) noexcept
{
	InitializeAllocation(count);
	UninitializedConstructFillCountPtr(m_ptrBegin, count, value);
	m_ptrEnd = m_ptrBegin + count;
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::Vector(const Vector<T, AllocatorType>& other) noexcept
{
	if constexpr (allocator_traits::RequiresAllocatorCopy)
	{
		allocator_type::CopyAllocator(this->get_allocator(), other.get_allocator());
	}

	InitializeAllocation(other.size());
	m_ptrEnd = UninitializedCopyPtr(other.m_ptrBegin, other.m_ptrEnd, m_ptrBegin);
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::Vector(Vector<T, AllocatorType>&& other) noexcept
{
	if constexpr (allocator_traits::RequiresAllocatorCopy)
	{
		allocator_type::CopyAllocator(this->get_allocator(), other.get_allocator());
	}

	swap(other);
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::Vector(std::initializer_list<T> initList) noexcept
{
	Initialize(initList.begin(), initList.end(), std::false_type());
}

template<typename T, typename AllocatorType>
template<typename InputIterator>
inline constexpr Vector<T, AllocatorType>::Vector(InputIterator begin, InputIterator end) noexcept
{
	Initialize(begin, end, std::is_integral<InputIterator>());
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::~Vector()
{
	Destruct(m_ptrBegin, m_ptrEnd);
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>& Vector<T, AllocatorType>::operator=(const Vector<T, AllocatorType>& rhs) noexcept
{
	if (this != &rhs)
	{
		if constexpr (allocator_traits::RequiresAllocatorCopy)
		{
			allocator_type::CopyAllocator(this->get_allocator(), rhs.get_allocator());
		}

		assign<const_iterator, false>(rhs.begin(), rhs.end(), std::false_type());
	}

	return *this;
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>& Vector<T, AllocatorType>::operator=(std::initializer_list<T> initList) noexcept
{
	typedef typename std::initializer_list<value_type>::iterator InputIterator;
	typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
	AssignFromIterator<InputIterator, false>(initList.begin(), initList.end(), IC());
	return *this;
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>& Vector<T, AllocatorType>::operator=(Vector<T, AllocatorType>&& other) noexcept
{
	if (this != &other)
	{
		if constexpr (allocator_traits::RequiresAllocatorCopy)
		{
			allocator_type::CopyAllocator(this->get_allocator(), other.get_allocator());
		}

		ClearCapacity();
		swap(other);
	}

	return *this;
}

template<typename T, typename AllocatorType>
inline constexpr void Vector<T, AllocatorType>::swap(Vector<T, AllocatorType>& other)
{
	const size_t thisSize = size();
	const size_t otherSize = other.size();

	const size_t thisCapacity = capacity();
	const size_t otherCapacity = other.capacity();

	InternalAllocator().Swap(other.InternalAllocator());

	m_ptrBegin = InternalAllocator().GetAllocation();
	m_ptrEnd = m_ptrBegin + otherSize;
	InternalCapacityPtr() = m_ptrBegin + otherCapacity;

	other.m_ptrBegin = other.InternalAllocator().GetAllocation();
	other.m_ptrEnd = other.m_ptrBegin + thisSize;
	other.InternalCapacityPtr() = m_ptrBegin + thisCapacity;
}

template<typename T, typename AllocatorType>
inline constexpr void Vector<T, AllocatorType>::assign(size_type count, const value_type& value)
{
	AssignValues(count, value);
}

template<typename T, typename AllocatorType>
template<typename InputIterator>
inline constexpr void Vector<T, AllocatorType>::assign(InputIterator first, InputIterator last)
{
	assign<InputIterator, false>(first, last, std::is_integral<InputIterator>());
}

template<typename T, typename AllocatorType>
inline constexpr void Vector<T, AllocatorType>::assign(std::initializer_list<value_type> initList)
{
	typedef typename std::initializer_list<value_type>::iterator InputIterator;
	typedef typename std::iterator_traits<InputIterator>::iterator_category IC;

	AssignFromIterator<InputIterator, false>(initList.begin(), initList.end(), IC());
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::iterator Vector<T, AllocatorType>::append(const Vector<T, AllocatorType>& other) noexcept
{
	return insert(end(), other.begin(), other.end());
}

template<typename T, typename AllocatorType>
template<typename InputIterator>
constexpr Vector<T, AllocatorType>::iterator Vector<T, AllocatorType>::append(InputIterator first, InputIterator last) noexcept
{
	return insert(end(), first, last);
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::iterator Vector<T, AllocatorType>::begin() noexcept
{
	return m_ptrBegin;
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::const_iterator Vector<T, AllocatorType>::begin() const noexcept
{
	return m_ptrBegin;
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::const_iterator Vector<T, AllocatorType>::cbegin() const noexcept
{
	return m_ptrBegin;
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::iterator Vector<T, AllocatorType>::end() noexcept
{
	return m_ptrEnd;
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::const_iterator Vector<T, AllocatorType>::end() const noexcept
{
	return m_ptrEnd;
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::const_iterator Vector<T, AllocatorType>::cend() const noexcept
{
	return m_ptrEnd;
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::reverse_iterator Vector<T, AllocatorType>::rbegin() noexcept
{
	return std::reverse_iterator(m_ptrEnd);
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::const_reverse_iterator Vector<T, AllocatorType>::rbegin() const noexcept
{
	return std::reverse_iterator(m_ptrEnd);
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::const_reverse_iterator Vector<T, AllocatorType>::crbegin() const noexcept
{
	return std::reverse_iterator(m_ptrEnd);
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::reverse_iterator Vector<T, AllocatorType>::rend() noexcept
{
	return std::reverse_iterator(m_ptrBegin);
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::const_reverse_iterator Vector<T, AllocatorType>::rend() const noexcept
{
	return std::reverse_iterator(m_ptrBegin);
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::const_reverse_iterator Vector<T, AllocatorType>::crend() const noexcept
{
	return std::reverse_iterator(m_ptrBegin);
}

template<typename T, typename AllocatorType>
inline constexpr bool Vector<T, AllocatorType>::empty() const noexcept
{
	return (m_ptrBegin == m_ptrEnd);
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::size_type Vector<T, AllocatorType>::size() const noexcept
{
	return static_cast<size_type>(m_ptrEnd - m_ptrBegin);
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::size_type Vector<T, AllocatorType>::byte_size() const noexcept
{
	return static_cast<size_type>((m_ptrEnd - m_ptrBegin) * sizeof(T));
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::size_type Vector<T, AllocatorType>::capacity() const noexcept
{
	return static_cast<size_type>(InternalCapacityPtr() - m_ptrBegin);
}

template<typename T, typename AllocatorType>
inline constexpr void Vector<T, AllocatorType>::resize(size_type count, const value_type& value)
{
	if (count > static_cast<size_type>(m_ptrEnd - m_ptrBegin))
	{
		InsertValuesAtEnd(count - (static_cast<size_type>(m_ptrEnd - m_ptrBegin)), value);
	}
	else
	{
		Destruct(m_ptrBegin + count, m_ptrEnd);
		m_ptrEnd = m_ptrBegin + count;
	}
}

template<typename T, typename AllocatorType>
inline constexpr void Vector<T, AllocatorType>::resize(size_type count)
{
	if (count > static_cast<size_type>(m_ptrEnd - m_ptrBegin))
	{
		InsertValuesAtEnd(count - (static_cast<size_type>(m_ptrEnd - m_ptrBegin)));
	}
	else
	{
		Destruct(m_ptrBegin + count, m_ptrEnd);
		m_ptrEnd = m_ptrBegin + count;
	}
}

template<typename T, typename AllocatorType>
inline constexpr void Vector<T, AllocatorType>::resize_uninitialized(size_type count)
{
	static_assert(std::is_trivially_destructible_v<T>);

	if (count > static_cast<size_type>(InternalCapacityPtr() - m_ptrBegin))
	{
		const size_type prevCount = static_cast<size_type>(m_ptrEnd - m_ptrBegin);
		const size_type growCount = GetNewCapacity(prevCount);
		const size_type newCount = std::max(growCount, prevCount + count);

		Grow(newCount);
	}

	m_ptrEnd = m_ptrBegin + count;
}

template<typename T, typename AllocatorType>
inline constexpr void Vector<T, AllocatorType>::reserve(size_type count)
{
	if (count > static_cast<size_type>(InternalCapacityPtr() - m_ptrBegin))
	{
		Grow(count);
	}
}

template<typename T, typename AllocatorType>
inline constexpr void Vector<T, AllocatorType>::set_capacity(size_type count)
{
	if ((count == npos) || (count <= static_cast<size_type>(m_ptrEnd - m_ptrBegin)))
	{
		if (count == 0)
		{
			clear();
		}
		else if (count < static_cast<size_type>(m_ptrEnd - m_ptrBegin))
		{
			resize(count);
		}

		shrink_to_fit();
	}
	else
	{
		value_type* const newData = Reallocate(count, m_ptrBegin, m_ptrEnd, ShouldMoveTag());
		if (newData == m_ptrBegin)
		{
			Destruct(m_ptrBegin, m_ptrEnd);
		}

		DoFree(m_ptrBegin);

		const ptrdiff_t prevCount = m_ptrEnd - m_ptrBegin;
		m_ptrBegin = newData;
		m_ptrEnd = newData + prevCount;
		InternalCapacityPtr() = m_ptrBegin + count;
	}
}

template<typename T, typename AllocatorType>
inline constexpr void Vector<T, AllocatorType>::shrink_to_fit()
{
	Vector<T, AllocatorType> temp = Vector<T, AllocatorType>(std::move_iterator<iterator>(begin()), std::move_iterator<iterator>(end()));
	swap(temp);
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::value_type* Vector<T, AllocatorType>::data() noexcept
{
	return m_ptrBegin;
}

template<typename T, typename AllocatorType>
inline constexpr const Vector<T, AllocatorType>::value_type* Vector<T, AllocatorType>::data() const noexcept
{
	return m_ptrBegin;
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::value_type& Vector<T, AllocatorType>::operator[](size_type position)
{
	VT_ASSERT_MSG(position < static_cast<size_type>(m_ptrEnd - m_ptrBegin), "Vector::operator[] - Out of range!");
	return *(m_ptrBegin + position);
}

template<typename T, typename AllocatorType>
inline constexpr const Vector<T, AllocatorType>::value_type& Vector<T, AllocatorType>::operator[](size_type position) const
{
	VT_ASSERT_MSG(position < static_cast<size_type>(m_ptrEnd - m_ptrBegin), "Vector::operator[] - Out of range!");
	return *(m_ptrBegin + position);
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::value_type& Vector<T, AllocatorType>::at(size_type position)
{
	VT_ASSERT_MSG(position < static_cast<size_type>(m_ptrEnd - m_ptrBegin), "Vector::At - Out of range!");
	return *(m_ptrBegin + position);
}

template<typename T, typename AllocatorType>
inline constexpr const Vector<T, AllocatorType>::value_type& Vector<T, AllocatorType>::at(size_type position) const
{
	VT_ASSERT_MSG(position < static_cast<size_type>(m_ptrEnd - m_ptrBegin), "Vector::At - Out of range!");
	return *(m_ptrBegin + position);
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::value_type& Vector<T, AllocatorType>::front()
{
	VT_ASSERT_MSG((m_ptrBegin != nullptr) && (m_ptrEnd > m_ptrBegin), "Vector::Front - Empty Vector!");
	return *m_ptrBegin;
}

template<typename T, typename AllocatorType>
inline constexpr const Vector<T, AllocatorType>::value_type& Vector<T, AllocatorType>::front() const
{
	VT_ASSERT_MSG((m_ptrBegin != nullptr) && (m_ptrEnd > m_ptrBegin), "Vector::Front - Empty Vector!");
	return *m_ptrBegin;
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::value_type& Vector<T, AllocatorType>::back()
{
	VT_ASSERT_MSG((m_ptrBegin != nullptr) && (m_ptrEnd > m_ptrBegin), "Vector::Back - Empty Vector!");
	return *(m_ptrEnd - 1);
}

template<typename T, typename AllocatorType>
inline constexpr const Vector<T, AllocatorType>::value_type& Vector<T, AllocatorType>::back() const
{
	VT_ASSERT_MSG((m_ptrBegin != nullptr) && (m_ptrEnd > m_ptrBegin), "Vector::Back - Empty Vector!");
	return *(m_ptrEnd - 1);
}

template<typename T, typename AllocatorType>
inline constexpr void Vector<T, AllocatorType>::push_back(const value_type& value)
{
	if (m_ptrEnd < InternalCapacityPtr())
	{
		::new((void*)m_ptrEnd++) T(value);
	}
	else
	{
		InsertValueAtEnd(std::move(value));
	}
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::value_type& Vector<T, AllocatorType>::push_back()
{
	if (m_ptrEnd < InternalCapacityPtr())
	{
		::new(static_cast<void*>(m_ptrEnd++)) value_type();
	}
	else
	{
		InsertValueAtEnd(value_type());
	}

	return *(m_ptrEnd - 1);
}

template<typename T, typename AllocatorType>
inline constexpr void Vector<T, AllocatorType>::push_back(value_type&& value)
{
	if (m_ptrEnd < InternalCapacityPtr())
	{
		::new(static_cast<void*>(m_ptrEnd++)) value_type(std::move(value));
	}
	else
	{
		InsertValueAtEnd(std::move(value));
	}
}

template<typename T, typename AllocatorType>
inline constexpr void Vector<T, AllocatorType>::pop_back()
{
	VT_ASSERT_MSG(m_ptrEnd != m_ptrBegin, "Vector::PopBack - Empty Vector!");

	--m_ptrEnd;
	m_ptrEnd->~value_type();
}

template<typename T, typename AllocatorType>
template<typename ...Args>
inline constexpr Vector<T, AllocatorType>::iterator Vector<T, AllocatorType>::emplace(const_iterator position, Args&& ...args)
{
	const ptrdiff_t count = position - m_ptrBegin;

	if ((m_ptrEnd == InternalCapacityPtr()) || (position != m_ptrEnd))
	{
		InsertValue(position, std::forward<Args>(args)...);
	}
	else
	{
		::new(static_cast<void*>(m_ptrEnd)) value_type(std::forward<Args>(args)...);
		++m_ptrEnd;
	}

	return m_ptrBegin + count;
}

template<typename T, typename AllocatorType>
template<typename ...Args>
inline constexpr Vector<T, AllocatorType>::value_type& Vector<T, AllocatorType>::emplace_back(Args && ...args)
{
	if (m_ptrEnd < InternalCapacityPtr())
	{
		::new(static_cast<void*>(m_ptrEnd)) value_type(std::forward<Args>(args)...);
		++m_ptrEnd;
	}
	else
	{
		InsertValueAtEnd(std::forward<Args>(args)...);
	}

	return back();
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::iterator Vector<T, AllocatorType>::insert(const_iterator position, const value_type& value)
{
	VT_ASSERT_MSG((position >= m_ptrBegin) || (position <= m_ptrEnd), "Vector::Insert - Invalid position!");

	const ptrdiff_t count = position - m_ptrBegin;

	if ((m_ptrEnd == InternalCapacityPtr()) || (position != m_ptrEnd))
	{
		InsertValue(position, value);
	}
	else
	{
		::new(static_cast<void*>(m_ptrEnd)) value_type(value);
		++m_ptrEnd;
	}

	return m_ptrBegin + count;
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::iterator Vector<T, AllocatorType>::insert(const_iterator position, value_type&& value)
{
	return emplace(position, std::move(value));
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::iterator Vector<T, AllocatorType>::insert(const_iterator position, size_type count, const value_type& value)
{
	const ptrdiff_t p = position - m_ptrBegin;
	InsertValues(position, count, value);

	return m_ptrBegin + p;
}

template<typename T, typename AllocatorType>
template<typename InputIterator>
inline constexpr Vector<T, AllocatorType>::iterator Vector<T, AllocatorType>::insert(const_iterator position, InputIterator first, InputIterator last)
{
	const ptrdiff_t p = position - m_ptrBegin;
	insert(position, first, last, std::is_integral<InputIterator>());

	return m_ptrBegin + p;
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::iterator Vector<T, AllocatorType>::insert(const_iterator position, std::initializer_list<value_type> initList)
{
	const ptrdiff_t p = position - m_ptrBegin;
	insert(position, initList.begin(), initList.end(), std::false_type());

	return m_ptrBegin + p;
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::iterator Vector<T, AllocatorType>::erase_first(const T& value)
{
	static_assert(HasEqualityV<T>, "T must be comparable!");

	iterator it = std::find(begin(), end(), value);

	if (it != end())
	{
		return erase(it);
	}
	else
	{
		return it;
	}
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::iterator Vector<T, AllocatorType>::erase_first_unsorted(const T& value)
{
	static_assert(HasEqualityV<T>, "T must be comparable!");

	iterator it = std::find(begin(), end(), value);

	if (it != end())
	{
		return erase_unsorted(it);
	}
	else
	{
		return it;
	}
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::reverse_iterator Vector<T, AllocatorType>::erase_last(const T& value)
{
	static_assert(HasEqualityV<T>, "T must be comparable!");

	reverse_iterator it = std::find(rbegin(), rend(), value);
	if (it != rend())
	{
		return erase(it);
	}
	else
	{
		return it;
	}
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::reverse_iterator Vector<T, AllocatorType>::erase_last_unsorted(const T& value)
{
	reverse_iterator it = std::find(rbegin(), rend(), value);
	if (it != rend())
	{
		return erase_unsorted(it);
	}
	else
	{
		return it;
	}
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::iterator Vector<T, AllocatorType>::erase(const_iterator position)
{
	VT_ASSERT_MSG((position >= m_ptrBegin) && (position < m_ptrEnd), "Vector::Erase - Invalid position!");

	iterator destPosition = const_cast<value_type*>(position);

	if ((position + 1) < m_ptrEnd)
	{
		std::move(destPosition + 1, m_ptrEnd, destPosition);
	}

	--m_ptrEnd;
	m_ptrEnd->~value_type();
	return destPosition;
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::iterator Vector<T, AllocatorType>::erase(const_iterator first, const_iterator last)
{
	VT_ASSERT_MSG((first >= m_ptrBegin) && (first < m_ptrEnd) && (last > m_ptrBegin) && (last <= m_ptrEnd) && (last > first), "Vector::Erase - Invalid position!");

	if (first != last)
	{
		const iterator position = const_cast<T*>(std::move(const_cast<T*>(last), const_cast<T*>(m_ptrEnd), const_cast<T*>(first)));
		Destruct(position, m_ptrEnd);
		m_ptrEnd -= (last - first);
	}

	return const_cast<T*>(first);
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::iterator Vector<T, AllocatorType>::erase_unsorted(const_iterator position)
{
	VT_ASSERT_MSG((position >= m_ptrBegin) && (position < m_ptrEnd), "Vector::EraseUnsorted - Invalid position!");

	iterator destPosition = const_cast<value_type*>(position);
	*destPosition = std::move(*(m_ptrEnd - 1));

	--m_ptrEnd;
	m_ptrEnd->~value_type();

	return destPosition;
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::reverse_iterator Vector<T, AllocatorType>::erase(const_reverse_iterator position)
{
	return reverse_iterator(erase((++position).base()));
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::reverse_iterator Vector<T, AllocatorType>::erase(const_reverse_iterator first, const_reverse_iterator last)
{
	return reverse_iterator(erase(last.base(), first.base()));
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::reverse_iterator Vector<T, AllocatorType>::erase_unsorted(const_reverse_iterator position)
{
	return reverse_iterator(erase_unsorted((++position).base()));
}

template<typename T, typename AllocatorType>
template<typename PredicateFunctor>
inline constexpr void Vector<T, AllocatorType>::erase_with_predicate(PredicateFunctor&& functor)
{
	for (auto it = rbegin(); it != rend(); ++it)
	{
		if (functor(*it))
		{
			erase(it);
		}
	}
}

template<typename T, typename AllocatorType>
constexpr bool Vector<T, AllocatorType>::contains(const T& value) const
{
	for (auto it = cbegin(); it != cend(); ++it)
	{
		if (value == (*it))
		{
			return true;
		}
	}

	return false;
}

template<typename T, typename AllocatorType>
template<typename PredicateFunctor>
inline constexpr bool Vector<T, AllocatorType>::contains_with_predicate(PredicateFunctor&& functor) const
{
	for (auto it = cbegin(); it != cend(); ++it)
	{
		if (functor(*it))
		{
			return true;
		}
	}

	return false;
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::iterator Vector<T, AllocatorType>::find(const T& value)
{
	for (auto it = begin(); it != end(); ++it)
	{
		if (value == (*it))
		{
			return it;
		}
	}

	return end();
}

template<typename T, typename AllocatorType>
inline constexpr Vector<T, AllocatorType>::const_iterator Vector<T, AllocatorType>::find(const T& value) const
{
	for (auto it = cbegin(); it != cend(); ++it)
	{
		if (value == (*it))
		{
			return it;
		}
	}

	return cend();
}

template<typename T, typename AllocatorType>
template<typename PredicateFunctor>
inline constexpr Vector<T, AllocatorType>::iterator Vector<T, AllocatorType>::find_with_predicate(PredicateFunctor&& functor)
{
	for (auto it = begin(); it != end(); ++it)
	{
		if (functor(*it))
		{
			return it;
		}
	}

	return end();
}

template<typename T, typename AllocatorType>
template<typename PredicateFunctor>
inline constexpr Vector<T, AllocatorType>::const_iterator Vector<T, AllocatorType>::find_with_predicate(PredicateFunctor&& functor) const
{
	for (auto it = cbegin(); it != cend(); ++it)
	{
		if (functor(*it))
		{
			return it;
		}
	}

	return cend();
}

template<typename T, typename AllocatorType>
inline constexpr void Vector<T, AllocatorType>::clear() noexcept
{
	Destruct(m_ptrBegin, m_ptrEnd);
	m_ptrEnd = m_ptrBegin;
}

template<typename T, typename AllocatorType>
inline void Vector<T, AllocatorType>::InsertValuesAtEnd(size_type count, const value_type& value)
{
	if (count > static_cast<size_type>(InternalCapacityPtr() - m_ptrEnd))
	{
		const size_type prevCount = static_cast<size_type>(m_ptrEnd - m_ptrBegin);
		const size_type growCount = GetNewCapacity(prevCount);
		const size_type newCount = std::max(growCount, prevCount + count);
		value_type* const newData = Allocate(newCount);

		if (newData == m_ptrBegin)
		{
			UninitializedConstructFillCountPtr(m_ptrEnd, count, value);
			m_ptrEnd += count;
		}
		else
		{
			value_type* newEnd = std::uninitialized_move(m_ptrBegin, m_ptrEnd, newData);

			UninitializedConstructFillCountPtr(newEnd, count, value);
			newEnd += count;

			Destruct(m_ptrBegin, m_ptrEnd);
			DoFree(m_ptrBegin);

			m_ptrBegin = newData;
			m_ptrEnd = newEnd;
		}

		InternalCapacityPtr() = newData + newCount;
	}
	else
	{
		UninitializedConstructFillCountPtr(m_ptrEnd, count, value);
		m_ptrEnd += count;
	}
}

template<typename T, typename AllocatorType>
inline void Vector<T, AllocatorType>::InsertValuesAtEnd(size_type count)
{
	if (count > static_cast<size_type>(InternalCapacityPtr() - m_ptrEnd))
	{
		const size_type prevCount = static_cast<size_type>(m_ptrEnd - m_ptrBegin);
		const size_type growCount = GetNewCapacity(prevCount);
		const size_type newCount = std::max(growCount, prevCount + count);
		value_type* const newData = Allocate(newCount);

		if (newData == m_ptrBegin)
		{
			UninitializedValueConstructCount(m_ptrEnd, count);
			m_ptrEnd += count;
		}
		else
		{
			value_type* newEnd = std::uninitialized_move(m_ptrBegin, m_ptrEnd, newData);

			UninitializedValueConstructCount(newEnd, count);
			newEnd += count;

			Destruct(m_ptrBegin, m_ptrEnd);
			DoFree(m_ptrBegin);

			m_ptrBegin = newData;
			m_ptrEnd = newEnd;
		}


		InternalCapacityPtr() = newData + newCount;
	}
	else
	{
		UninitializedValueConstructCount(m_ptrEnd, count);
		m_ptrEnd += count;
	}
}

template<typename T, typename AllocatorType>
inline void Vector<T, AllocatorType>::ClearCapacity()
{
	clear();
	Vector<T, AllocatorType> temp(std::move(*this));
	swap(temp);
}

template<typename T, typename AllocatorType>
inline void Vector<T, AllocatorType>::Grow(size_type count)
{
	value_type* const newData = Allocate(count);
	if (newData != m_ptrBegin)
	{
		value_type* newEnd = std::uninitialized_move(m_ptrBegin, m_ptrEnd, newData);

		Destruct(m_ptrBegin, m_ptrEnd);
		DoFree(m_ptrBegin);

		m_ptrBegin = newData;
		m_ptrEnd = newEnd;
	}

	InternalCapacityPtr() = newData + count;
}

template<typename T, typename AllocatorType>
inline Vector<T, AllocatorType>::size_type Vector<T, AllocatorType>::GetNewCapacity(size_type currentCapacity)
{
	return (currentCapacity > 0) ? (2 * currentCapacity) : 1;
}

template<typename T, typename AllocatorType>
inline void Vector<T, AllocatorType>::InitializeAllocation(size_type count)
{
	// Should not initialize empty allocations.
	if (count == 0)
	{
		return;
	}

	m_ptrBegin = Allocate(count);
	InternalCapacityPtr() = m_ptrBegin + count;
}

template<typename T, typename AllocatorType>
template<typename ForwardIterator>
inline Vector<T, AllocatorType>::value_type* Vector<T, AllocatorType>::Reallocate(size_type count, ForwardIterator first, ForwardIterator last, ShouldCopyTag)
{
	value_type* const ptr = Allocate(count);
	if (ptr != m_ptrBegin)
	{
		UninitializedCopyPtr(first, last, ptr);
	}
	return ptr;
}

template<typename T, typename AllocatorType>
template<typename ForwardIterator>
inline Vector<T, AllocatorType>::value_type* Vector<T, AllocatorType>::Reallocate(size_type count, ForwardIterator first, ForwardIterator last, ShouldMoveTag)
{
	value_type* const ptr = Allocate(count);
	if (ptr != m_ptrBegin)
	{
		std::uninitialized_move(first, last, ptr);
	}
	return ptr;
}

template<typename T, typename AllocatorType>
template<typename Integer>
inline void Vector<T, AllocatorType>::Initialize(Integer count, Integer value, std::true_type)
{
	InitializeAllocation(static_cast<size_type>(count));
	m_ptrEnd = InternalCapacityPtr();

	UninitializedConstructFillCountPtr<value_type, Integer>(m_ptrBegin, count, value);
}

template<typename T, typename AllocatorType>
template<typename InputIterator>
inline void Vector<T, AllocatorType>::Initialize(InputIterator begin, InputIterator end, std::false_type)
{
	typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
	InitializeFromIterator(begin, end, IC());
}

template<typename T, typename AllocatorType>
template<typename InputIterator>
inline void Vector<T, AllocatorType>::InitializeFromIterator(InputIterator begin, InputIterator end, std::input_iterator_tag)
{
	for (; begin != end; ++begin)
	{
		push_back(*begin);
	}
}

template<typename T, typename AllocatorType>
template<typename ForwardIterator>
inline void Vector<T, AllocatorType>::InitializeFromIterator(ForwardIterator begin, ForwardIterator end, std::forward_iterator_tag)
{
	const size_type count = static_cast<size_type>(std::distance(begin, end));
	InitializeAllocation(count);
	m_ptrEnd = InternalCapacityPtr();

	UninitializedCopyPtr(begin, end, m_ptrBegin);
}

template<typename T, typename AllocatorType>
template<typename Integer, bool move>
inline void Vector<T, AllocatorType>::assign(Integer n, Integer value, std::true_type)
{
	AssignValues(static_cast<size_type>(n), static_cast<T>(value));
}

template<typename T, typename AllocatorType>
template<typename InputIterator, bool move>
inline void Vector<T, AllocatorType>::assign(InputIterator begin, InputIterator end, std::false_type)
{
	typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
	AssignFromIterator<InputIterator, move>(begin, end, IC());
}

template<typename T, typename AllocatorType>
inline void Vector<T, AllocatorType>::AssignValues(size_type count, const T& value)
{
	if (count > static_cast<size_type>(InternalCapacityPtr() - m_ptrBegin))
	{
		// If there isn't enough capacity, we must re allocate
		Vector<T, AllocatorType> temp(count, value);
		swap(temp);
	}
	else if (count > static_cast<size_type>(m_ptrEnd - m_ptrBegin))
	{
		std::fill(m_ptrBegin, m_ptrEnd, value);
		UninitializedConstructFillCountPtr(m_ptrEnd, count - static_cast<size_type>(m_ptrEnd - m_ptrBegin), value);
		m_ptrEnd += count - static_cast<size_type>(m_ptrEnd - m_ptrBegin);
	}
	else
	{
		std::fill_n(m_ptrBegin, count, value);
		erase(m_ptrBegin + count, m_ptrEnd);
	}
}

template<typename T, typename AllocatorType>
template<typename InputIterator, bool move>
inline void Vector<T, AllocatorType>::AssignFromIterator(InputIterator begin, InputIterator end, std::input_iterator_tag)
{
	iterator position(m_ptrBegin);

	while ((position != m_ptrEnd) && (begin != end))
	{
		*position = *begin;
		++begin;
		++position;
	}

	if (begin == end)
	{
		erase(position, m_ptrEnd);
	}
	else
	{
		insert(m_ptrEnd, begin, end);
	}
}

template<typename T, typename AllocatorType>
template<typename RandomAccessIterator, bool move>
inline void Vector<T, AllocatorType>::AssignFromIterator(RandomAccessIterator begin, RandomAccessIterator end, std::random_access_iterator_tag)
{
	const size_type count = static_cast<size_type>(std::distance(begin, end));

	if (count > static_cast<size_type>(InternalCapacityPtr() - m_ptrBegin)) // count > capacity
	{
		value_type* const newData = Reallocate(count, begin, end, ShouldMoveOrCopyTag<move>());
		if (newData != m_ptrBegin)
		{
			Destruct(m_ptrBegin, m_ptrEnd);
		}
		else
		{
			std::copy(begin, end, newData);
		}
		DoFree(m_ptrBegin);

		m_ptrBegin = newData;
		m_ptrEnd = m_ptrBegin + count;
		InternalCapacityPtr() = m_ptrEnd;
	}
	else if (count <= static_cast<size_type>(m_ptrEnd - m_ptrBegin)) // count <= size
	{
		value_type* const newEnd = std::copy(begin, end, m_ptrBegin);
		Destruct(newEnd, m_ptrEnd);
		m_ptrEnd = newEnd;
	}
	else
	{
		RandomAccessIterator position = begin + (m_ptrEnd - m_ptrBegin);
		std::copy(begin, position, m_ptrBegin);
		m_ptrEnd = UninitializedCopyPtr(position, end, m_ptrEnd);
	}
}

template<typename T, typename AllocatorType>
template<typename Integer>
inline void Vector<T, AllocatorType>::insert(const_iterator position, Integer count, Integer value, std::true_type)
{
	InsertValues(position, static_cast<size_type>(count), static_cast<value_type>(value));
}

template<typename T, typename AllocatorType>
template<typename InputIterator>
inline void Vector<T, AllocatorType>::insert(const_iterator position, InputIterator first, InputIterator last, std::false_type)
{
	typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
	InsertFromIterator(position, first, last, IC());
}

template<typename T, typename AllocatorType>
template<typename InputIterator>
inline void Vector<T, AllocatorType>::InsertFromIterator(const_iterator position, InputIterator first, InputIterator last, std::input_iterator_tag)
{
	for (; first != last; ++first, ++position)
	{
		position = insert(position, *first);
	}
}

template<typename T, typename AllocatorType>
template<typename BidirectionalIterator>
inline void Vector<T, AllocatorType>::InsertFromIterator(const_iterator position, BidirectionalIterator first, BidirectionalIterator last, std::bidirectional_iterator_tag)
{
	VT_ASSERT_MSG((position >= m_ptrBegin) && (position <= m_ptrEnd), "Vector::InsertFromIterator - Invalid position!");

	iterator destPosition = const_cast<value_type*>(position);

	if (first != last)
	{
		const size_type count = static_cast<size_type>(std::distance(first, last));

		if (count <= static_cast<size_type>(InternalCapacityPtr() - m_ptrEnd))
		{
			const size_type countExtra = static_cast<size_type>(m_ptrEnd - destPosition);

			if (count < countExtra)
			{
				std::uninitialized_move(m_ptrEnd - count, m_ptrEnd, m_ptrEnd);
				std::move_backward(destPosition, m_ptrEnd - count, m_ptrEnd);
				std::copy(first, last, destPosition);
			}
			else
			{
				BidirectionalIterator tempIter = first;
				std::advance(tempIter, countExtra);
				UninitializedCopyPtr(tempIter, last, m_ptrEnd);
				std::uninitialized_move(destPosition, m_ptrEnd, m_ptrEnd + count - countExtra);
				std::copy_backward(first, tempIter, destPosition + countExtra);
			}

			m_ptrEnd += count;
		}
		else
		{
			const size_type prevCount = static_cast<size_type>(m_ptrEnd - m_ptrBegin);
			const size_type growCount = GetNewCapacity(prevCount);
			const size_type newCount = growCount > (prevCount + count) ? growCount : (prevCount + count);
			value_type* const newData = Allocate(newCount);

			const size_type destOffset = (destPosition - m_ptrBegin);
			const size_type copyCount = (last - first);
			const size_type secondPartitionCount = (m_ptrEnd - destPosition);

			value_type* secondPartitionDest = newData + destOffset + copyCount;
			value_type* copyDataDest = newData + destOffset;

			if (m_ptrBegin == newData)
			{
				const size_t numUninitializedMoves = copyCount - secondPartitionCount;
				const size_t numInitializedMoves = copyCount - numUninitializedMoves;
				if (numInitializedMoves > 0)
				{
					std::move(destPosition, destPosition + numInitializedMoves, secondPartitionDest);
				}
				std::uninitialized_move(destPosition + numInitializedMoves, destPosition + numInitializedMoves + numUninitializedMoves, secondPartitionDest + numInitializedMoves);
				UninitializedCopyPtr(first, last, copyDataDest);

				value_type* newEnd = secondPartitionDest + secondPartitionCount;
				m_ptrEnd = newEnd;
			}
			else
			{
				std::uninitialized_move(m_ptrBegin, destPosition, newData);
				std::uninitialized_move(destPosition, m_ptrEnd, secondPartitionDest);
				UninitializedCopyPtr(first, last, copyDataDest);
				value_type* newEnd = secondPartitionDest + secondPartitionCount;

				Destruct(m_ptrBegin, m_ptrEnd);
				DoFree(m_ptrBegin);

				m_ptrBegin = newData;
				m_ptrEnd = newEnd;
			}

			InternalCapacityPtr() = newData + newCount;
		}
	}
}

template<typename T, typename AllocatorType>
inline void Vector<T, AllocatorType>::InsertValues(const_iterator position, size_type count, const value_type& value)
{
	VT_ASSERT_MSG((position >= m_ptrBegin) && (position <= m_ptrEnd), "Vector::InsertValues - Invalid position!");

	iterator destPosition = const_cast<value_type*>(position);
	if (count <= static_cast<size_type>(InternalCapacityPtr() - m_ptrEnd)) // count <= capacity
	{
		if (count > 0)
		{
			const value_type temp = value;
			const size_type insertPosition = static_cast<size_type>(m_ptrEnd - destPosition);

			if (count < insertPosition)
			{
				std::uninitialized_move(m_ptrEnd - count, m_ptrEnd, m_ptrEnd);
				std::move_backward(destPosition, m_ptrEnd - count, m_ptrEnd);
				std::fill(destPosition, destPosition + count, temp);
			}
			else
			{
				UninitializedConstructFillCountPtr(m_ptrEnd, count - insertPosition, temp);
				std::uninitialized_move(destPosition, m_ptrEnd, m_ptrEnd + count - insertPosition);
				std::fill(destPosition, m_ptrEnd, temp);
			}

			m_ptrEnd += count;
		}
	}
	else // count > capacity
	{
		const size_type prevCount = static_cast<size_type>(m_ptrEnd - m_ptrBegin);
		const size_type growCount = GetNewCapacity(prevCount);
		const size_type newCount = growCount > (prevCount + count) ? growCount : (prevCount + count);
		value_type* const newData = Allocate(newCount);

		const size_type firstPartitionCount = destPosition - m_ptrBegin;
		const size_type secondPartitionCount = m_ptrEnd - destPosition;

		if (newData == m_ptrBegin)
		{
			value_type* newEnd = std::uninitialized_move(destPosition, m_ptrEnd, newData + firstPartitionCount + count);
			UninitializedConstructFillCountPtr(newData + firstPartitionCount, count, value);
			m_ptrEnd = newEnd;
		}
		else
		{
			std::uninitialized_move(m_ptrBegin, destPosition, newData);
			std::uninitialized_move(destPosition, m_ptrEnd, newData + firstPartitionCount + count);
			UninitializedConstructFillCountPtr(newData + firstPartitionCount, count, value);

			value_type* newEnd = newData + firstPartitionCount + count + secondPartitionCount;

			Destruct(m_ptrBegin, m_ptrEnd);
			DoFree(m_ptrBegin);

			m_ptrBegin = newData;
			m_ptrEnd = newEnd;
		}

		InternalCapacityPtr() = newData + newCount;
	}
}

template<typename T, typename AllocatorType>
template<typename ...Args>
inline void Vector<T, AllocatorType>::InsertValue(const_iterator position, Args && ...args)
{
	VT_ASSERT_MSG((position >= m_ptrBegin) || (position <= m_ptrEnd), "Vector::InsertValue - Invalid position!");

	iterator destPosition = const_cast<value_type*>(position);

	if (m_ptrEnd != InternalCapacityPtr()) // Size < Capacity
	{
		VT_ASSERT(position < m_ptrEnd);

		value_type value(std::forward<Args>(args)...);

		::new(static_cast<void*>(m_ptrEnd)) value_type(std::move(*(m_ptrEnd - 1))); // m_ptrEnd is unitialized memory, thus we need to construct it the value
		std::move_backward(destPosition, m_ptrEnd - 1, m_ptrEnd);
		Destruct(destPosition);
		::new(static_cast<void*>(destPosition)) value_type(std::move(value));
		++m_ptrEnd;
	}
	else
	{
		const size_type insertPos = size_type(destPosition - m_ptrBegin);
		const size_type prevCount = size_type(m_ptrEnd - m_ptrBegin);
		const size_type newCount = GetNewCapacity(prevCount);
		value_type* const newData = Allocate(newCount);

		if (newData == m_ptrBegin)
		{
			value_type* newEnd = std::uninitialized_move(destPosition, m_ptrEnd, newData + insertPos + 1);
			::new(static_cast<void*>(newData + insertPos)) value_type(std::forward<Args>(args)...);
			m_ptrEnd = newEnd;
		}	
		else
		{
			value_type* newEnd = std::uninitialized_move(m_ptrBegin, destPosition, newData);
			newEnd = std::uninitialized_move(destPosition, m_ptrEnd, ++newEnd);

			::new(static_cast<void*>(newData + insertPos)) value_type(std::forward<Args>(args)...);

			Destruct(m_ptrBegin, m_ptrEnd);
			DoFree(m_ptrBegin);

			m_ptrBegin = newData;
			m_ptrEnd = newEnd;
		}

		InternalCapacityPtr() = newData + newCount;
	}
}

template<typename T, typename AllocatorType>
template<typename ...Args>
inline void Vector<T, AllocatorType>::InsertValueAtEnd(Args && ...args)
{
	const size_type prevCount = static_cast<size_type>(m_ptrEnd - m_ptrBegin);
	const size_type newCount = GetNewCapacity(prevCount);
	T* const newData = Allocate(newCount);

	if (newData == m_ptrBegin)
	{
		::new((void*)m_ptrEnd) T(std::forward<Args>(args)...);
		m_ptrEnd++;
	}
	else
	{
		T* newEnd = std::uninitialized_move(m_ptrBegin, m_ptrEnd, newData);
		::new((void*)newEnd) T(std::forward<Args>(args)...);
		newEnd++;

		Destruct(m_ptrBegin, m_ptrEnd);
		DoFree(m_ptrBegin);

		m_ptrBegin = newData;
		m_ptrEnd = newEnd;
	}

	InternalCapacityPtr() = newData + newCount;
}

template<typename T, typename Allocator>
inline VectorBase<T, Allocator>::VectorBase()
	: m_ptrBegin(nullptr),
	m_ptrEnd(nullptr),
	m_capacityAllocator(nullptr, allocator_type())
{}

template<typename T, typename Allocator>
inline VectorBase<T, Allocator>::VectorBase(const allocator_type& allocator)
	: m_ptrBegin(nullptr),
	m_ptrEnd(nullptr),
	m_capacityAllocator(nullptr, allocator)
{}

template<typename T, typename Allocator>
inline VectorBase<T, Allocator>::VectorBase(size_type num, const allocator_type& allocator)
{
	m_ptrBegin = Allocate(num);
	m_ptrEnd = m_ptrBegin;
	InternalCapacityPtr() = m_ptrBegin + num;
}

template<typename T, typename Allocator>
inline VectorBase<T, Allocator>::~VectorBase()
{
	if (m_ptrBegin)
	{
		DoFree(m_ptrBegin);
	}
}

template<typename T, typename Allocator>
inline const VectorBase<T, Allocator>::allocator_type& VectorBase<T, Allocator>::get_allocator() const noexcept
{
	return InternalAllocator();
}

template<typename T, typename Allocator>
inline VectorBase<T, Allocator>::allocator_type& VectorBase<T, Allocator>::get_allocator() noexcept
{
	return InternalAllocator();
}

template<typename T, typename Allocator>
inline void VectorBase<T, Allocator>::set_allocator(const allocator_type& allocator)
{
	InternalAllocator() = allocator;
}

template<typename T, typename Allocator>
inline T* VectorBase<T, Allocator>::Allocate(size_type num)
{
	constexpr size_t alignment = alignof(T);
	const size_t size = sizeof(T) * num;

	void* ptr = InternalAllocator().Allocate(size, alignment);
	return reinterpret_cast<T*>(ptr);
}

template<typename T, typename Allocator>
inline void VectorBase<T, Allocator>::DoFree(T* ptr)
{
	InternalAllocator().Free(ptr);
}
