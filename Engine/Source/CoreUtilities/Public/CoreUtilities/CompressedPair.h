#pragma once

#include "CallTraits.h"

// License in Engine\Source\ThirdParty\eastl

template<typename T1, typename T2>
class CompressedPair;

template<typename T1, typename T2, bool IsSame, bool FirstEmpty, bool SecondEmpty>
struct CompressedPairSwitch;

template<typename T1, typename T2>
struct CompressedPairSwitch<T1, T2, false, false, false> { static constexpr const int value = 0; };

template<typename T1, typename T2>
struct CompressedPairSwitch<T1, T2, false, true, false> { static constexpr const int value = 1; };

template<typename T1, typename T2>
struct CompressedPairSwitch<T1, T2, false, false, true> { static constexpr const int value = 2; };

template<typename T1, typename T2>
struct CompressedPairSwitch<T1, T2, false, true, true> { static constexpr const int value = 3; };

template<typename T1, typename T2>
struct CompressedPairSwitch<T1, T2, true, true, true> { static constexpr const int value = 4; };

template<typename T1, typename T2>
struct CompressedPairSwitch<T1, T2, true, false, false> { static constexpr const int value = 5; };

template<typename T1, typename T2, int Version>
class CompressedPairImp;

template<typename T>
inline void CompressedPairSwap(T& t1, T& t2)
{
	T temp = t1;
	t1 = t2;
	t2 = temp;
}

template<typename T1, typename T2>
class CompressedPairImp<T1, T2, 0>
{
public:
	typedef T1 FirstType;
	typedef T2 SecondType;
	typedef typename CallTraits<FirstType>::ParamType FirstParamType;
	typedef typename CallTraits<SecondType>::ParamType SecondParamType;
	typedef typename CallTraits<FirstType>::Reference FirstReference;
	typedef typename CallTraits<SecondType>::Reference SecondReference;
	typedef typename CallTraits<FirstType>::ConstReference FirstConstReference;
	typedef typename CallTraits<SecondType>::ConstReference SecondConstReference;

	CompressedPairImp() {}
	CompressedPairImp(FirstParamType x, SecondParamType y)
		: m_first(x), m_second(y)
	{ }

	CompressedPairImp(FirstParamType x)
		: m_first(x)
	{ }

	CompressedPairImp(SecondParamType y)
		: m_second(y)
	{ }

	FirstReference First() { return m_first; }
	FirstConstReference First() const { return m_first; }

	SecondReference Second() { return m_second; }
	SecondConstReference Second() const { return m_second; }

	void Swap(CompressedPair<T1, T2>& y)
	{
		CompressedPairSwap(m_first, y.First());
		CompressedPairSwap(m_second, y.Second());
	}

private:
	FirstType m_first;
	SecondType m_second;
};

template<typename T1, typename T2>
class CompressedPairImp<T1, T2, 1> : private T1
{
public:
	typedef T1 FirstType;
	typedef T2 SecondType;
	typedef typename CallTraits<FirstType>::ParamType FirstParamType;
	typedef typename CallTraits<SecondType>::ParamType SecondParamType;
	typedef typename CallTraits<FirstType>::Reference FirstReference;
	typedef typename CallTraits<SecondType>::Reference SecondReference;
	typedef typename CallTraits<FirstType>::ConstReference FirstConstReference;
	typedef typename CallTraits<SecondType>::ConstReference SecondConstReference;

	CompressedPairImp() {}
	CompressedPairImp(FirstParamType x, SecondParamType y)
		: FirstType(x), m_second(y)
	{
	}

	CompressedPairImp(FirstParamType x)
		: FirstType(x)
	{
	}

	CompressedPairImp(SecondParamType y)
		: m_second(y)
	{
	}

	FirstReference First() { return *this; }
	FirstConstReference First() const { return *this; }

	SecondReference Second() { return m_second; }
	SecondConstReference Second() const { return m_second; }

	void Swap(CompressedPair<T1, T2>& y)
	{
		CompressedPairSwap(m_second, y.Second());
	}

private:
	SecondType m_second;
};

template<typename T1, typename T2>
class CompressedPairImp<T1, T2, 2> : private T2
{
public:
	typedef T1 FirstType;
	typedef T2 SecondType;
	typedef typename CallTraits<FirstType>::ParamType FirstParamType;
	typedef typename CallTraits<SecondType>::ParamType SecondParamType;
	typedef typename CallTraits<FirstType>::Reference FirstReference;
	typedef typename CallTraits<SecondType>::Reference SecondReference;
	typedef typename CallTraits<FirstType>::ConstReference FirstConstReference;
	typedef typename CallTraits<SecondType>::ConstReference SecondConstReference;

	CompressedPairImp() {}
	CompressedPairImp(FirstParamType x, SecondParamType y)
		: SecondType(y), m_first(x)
	{
	}

	CompressedPairImp(FirstParamType x)
		: m_first(x)
	{
	}

	CompressedPairImp(SecondParamType y)
		: SecondType(y)
	{
	}

	FirstReference First() { return m_first; }
	FirstConstReference First() const { return m_first; }

	SecondReference Second() { return *this; }
	SecondConstReference Second() const { return *this; }

	void Swap(CompressedPair<T1, T2>& y)
	{
		CompressedPairSwap(m_first, y.First());
	}

private:
	FirstType m_first;
};

template<typename T1, typename T2>
class CompressedPairImp<T1, T2, 3> : private T1, private T2
{
public:
	typedef T1 FirstType;
	typedef T2 SecondType;
	typedef typename CallTraits<FirstType>::ParamType FirstParamType;
	typedef typename CallTraits<SecondType>::ParamType SecondParamType;
	typedef typename CallTraits<FirstType>::Reference FirstReference;
	typedef typename CallTraits<SecondType>::Reference SecondReference;
	typedef typename CallTraits<FirstType>::ConstReference FirstConstReference;
	typedef typename CallTraits<SecondType>::ConstReference SecondConstReference;

	CompressedPairImp() {}
	CompressedPairImp(FirstParamType x, SecondParamType y)
		: FirstType(x), SecondType(y)
	{
	}

	CompressedPairImp(FirstParamType x)
		: FirstType(x)
	{
	}

	CompressedPairImp(SecondParamType y)
		: SecondType(y)
	{
	}

	FirstReference First() { return *this; }
	FirstConstReference First() const { return *this; }

	SecondReference Second() { return *this; }
	SecondConstReference Second() const { return *this; }

	void Swap(CompressedPair<T1, T2>& y)
	{}
};

template<typename T1, typename T2>
class CompressedPairImp<T1, T2, 4> : private T1
{
public:
	typedef T1 FirstType;
	typedef T2 SecondType;
	typedef typename CallTraits<FirstType>::ParamType FirstParamType;
	typedef typename CallTraits<SecondType>::ParamType SecondParamType;
	typedef typename CallTraits<FirstType>::Reference FirstReference;
	typedef typename CallTraits<SecondType>::Reference SecondReference;
	typedef typename CallTraits<FirstType>::ConstReference FirstConstReference;
	typedef typename CallTraits<SecondType>::ConstReference SecondConstReference;

	CompressedPairImp() {}
	CompressedPairImp(FirstParamType x, SecondParamType y)
		: FirstType(x), SecondType(y)
	{
	}

	CompressedPairImp(FirstParamType x)
		: FirstType(x)
	{
	}

	FirstReference First() { return *this; }
	FirstConstReference First() const { return *this; }

	SecondReference Second() { return *this; }
	SecondConstReference Second() const { return *this; }

	void Swap(CompressedPair<T1, T2>& y)
	{
	}
};

template<typename T1, typename T2>
class CompressedPairImp<T1, T2, 5>
{
public:
	typedef T1 FirstType;
	typedef T2 SecondType;
	typedef typename CallTraits<FirstType>::ParamType FirstParamType;
	typedef typename CallTraits<SecondType>::ParamType SecondParamType;
	typedef typename CallTraits<FirstType>::Reference FirstReference;
	typedef typename CallTraits<SecondType>::Reference SecondReference;
	typedef typename CallTraits<FirstType>::ConstReference FirstConstReference;
	typedef typename CallTraits<SecondType>::ConstReference SecondConstReference;

	CompressedPairImp() {}
	CompressedPairImp(FirstParamType x, SecondParamType y)
		: m_first(x), m_second(y)
	{
	}

	CompressedPairImp(FirstParamType x)
		: m_first(x), m_second(x)
	{
	}

	FirstReference First() { return m_first; }
	FirstConstReference First() const { return m_first; }

	SecondReference Second() { return m_second; }
	SecondConstReference Second() const { return m_second; }

	void Swap(CompressedPair<T1, T2>& y)
	{
		CompressedPairSwap(m_first, y.First());
		CompressedPairSwap(m_second, y.Second());
	}

private:
	FirstType m_first;
	SecondType m_second;
};

template <typename T1, typename T2>
class CompressedPair
	: private CompressedPairImp<T1, T2,
	CompressedPairSwitch<
	T1,
	T2,
	std::is_same<typename std::remove_cv<T1>::type, typename std::remove_cv<T2>::type>::value,
	std::is_empty<T1>::value,
	std::is_empty<T2>::value>::value>
{
private:
	typedef CompressedPairImp <T1, T2,
		CompressedPairSwitch<
		T1,
		T2,
		std::is_same<typename std::remove_cv<T1>::type, typename std::remove_cv<T2>::type>::value,
		std::is_empty<T1>::value,
		std::is_empty<T2>::value>::value> Base;
public:
	typedef T1 FirstType;
	typedef T2 SecondType;
	typedef typename CallTraits<FirstType>::ParamType FirstParamType;
	typedef typename CallTraits<SecondType>::ParamType SecondParamType;
	typedef typename CallTraits<FirstType>::Reference FirstReference;
	typedef typename CallTraits<SecondType>::Reference SecondReference;
	typedef typename CallTraits<FirstType>::ConstReference FirstConstReference;
	typedef typename CallTraits<SecondType>::ConstReference SecondConstReference;

	CompressedPair() : Base() {}
	CompressedPair(FirstParamType x, SecondParamType y) : Base(x, y) {}
	explicit CompressedPair(FirstParamType x) : Base(x) {}
	explicit CompressedPair(SecondParamType y) : Base(y) {}

	FirstReference First() { return Base::First(); }
	FirstConstReference First() const { return Base::First(); }

	SecondReference Second() { return Base::Second(); }
	SecondConstReference Second() const { return Base::Second(); }

	void Swap(CompressedPair<T1, T2>& y) { Base::Swap(y); }
};

template <typename T>
class CompressedPair<T, T>
	: private CompressedPairImp<T, T,
	CompressedPairSwitch<
	T,
	T,
	std::is_same<typename std::remove_cv<T>::type, typename std::remove_cv<T>::type>::value,
	std::is_empty<T>::value,
	std::is_empty<T>::value>::value>
{
private:
	typedef CompressedPairImp <T, T,
		CompressedPairSwitch<
		T,
		T,
		std::is_same<typename std::remove_cv<T>::type, typename std::remove_cv<T>::type>::value,
		std::is_empty<T>::value,
		std::is_empty<T>::value>::value> Base;
public:
	typedef T FirstType;
	typedef T SecondType;
	typedef typename CallTraits<FirstType>::ParamType FirstParamType;
	typedef typename CallTraits<SecondType>::ParamType SecondParamType;
	typedef typename CallTraits<FirstType>::Reference FirstReference;
	typedef typename CallTraits<SecondType>::Reference SecondReference;
	typedef typename CallTraits<FirstType>::ConstReference FirstConstReference;
	typedef typename CallTraits<SecondType>::ConstReference SecondConstReference;

	CompressedPair() : Base() {}
	CompressedPair(FirstParamType x, SecondParamType y) : Base(x, y) {}
	explicit CompressedPair(FirstParamType x) : Base(x) {}

	FirstReference First() { return Base::First(); }
	FirstConstReference First() const { return Base::First(); }

	SecondReference Second() { return Base::Second(); }
	SecondConstReference Second() const { return Base::Second(); }

	void Swap(CompressedPair<T, T>& y) { Base::Swap(y); }
};
