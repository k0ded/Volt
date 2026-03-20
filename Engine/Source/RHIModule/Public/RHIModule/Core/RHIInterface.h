#pragma once

#include "RHIModule/Core/Core.h"

#include <CoreUtilities/Pointers/IntRefCounted.h>
#include <CoreUtilities/Pointers/IntRef.h>
#include <CoreUtilities/Pointers/ArenaIntRefCounted.h>

namespace Volt::RHI
{
	template<template<typename> class RefCounter = IntRefCounted>
	class TRHIInterface : public RefCounter<TRHIInterface<RefCounter>>
	{
	public:
		~TRHIInterface() override = default;
		VT_DELETE_COPY_MOVE(TRHIInterface);

		template<typename T>
		constexpr T GetHandle() const
		{
			return reinterpret_cast<T>(GetHandleImpl());
		}

		template<typename T>
		constexpr T* As()
		{
			return reinterpret_cast<T*>(this);
		}

		template<typename T>
		constexpr T& AsRef()
		{
			return *reinterpret_cast<T*>(this);
		}

	protected:
		TRHIInterface() = default;

		virtual void* GetHandleImpl() const = 0;
	};

	using RHIInterface = TRHIInterface<IntRefCounted>;
	using ArenaRHIInterface = TRHIInterface<ArenaIntRefCounted>;
}
