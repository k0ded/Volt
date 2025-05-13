#pragma once

#include <type_traits>

// License in Engine\Source\ThirdParty\eastl

template<typename T, bool Small>
struct CTImp2 { typedef const T& ParamType; };

template<typename T>
struct CTImp2<T, true> { typedef const T ParamType; };

template<typename T, bool ISP, bool B1>
struct CTImp { typedef const T& ParamType; };

template<typename T, bool ISP>
struct CTImp<T, ISP, true> { typedef typename CTImp2<T, sizeof(T) <= sizeof(void*)>::ParamType ParamType; };

template<typename T, bool B1>
struct CTImp<T, true, B1> { typedef T const ParamType; };

template<typename T>
struct CallTraits
{
public:
	typedef T ValueType;
	typedef T& Reference;
	typedef const T& ConstReference;
	typedef typename CTImp<T, std::is_pointer<T>::value, std::is_arithmetic<T>::value>::ParamType ParamType;
};

template<typename T>
struct CallTraits<T&>
{
public:
	typedef T& ValueType;
	typedef T& Reference;
	typedef const T& ConstReference;
	typedef T& ParamType;
};

template<typename T, size_t N>
struct CallTraits<T [N]>
{
private:
	typedef T ArrayType[N];

public:
	typedef const T* ValueType;
	typedef ArrayType& Reference;
	typedef const ArrayType& ConstReference;
	typedef const T* ParamType;
};

template<typename T, size_t N>
struct CallTraits<const T[N]>
{
private:
	typedef const T ArrayType[N];

public:
	typedef const T* ValueType;
	typedef ArrayType& Reference;
	typedef const ArrayType& ConstReference;
	typedef const T* ParamType;
};
