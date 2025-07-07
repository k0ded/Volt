#pragma once

#include "RHIModule/Core/Core.h"

#include <CoreUtilities/Pointers/RefCounted.h>
#include <CoreUtilities/Pointers/RefPtr.h>
#include <CoreUtilities/Pointers/ArenaRefCounted.h>

namespace Volt::RHI
{
	template<template<typename> class RefCounter = RefCounted>
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

	using RHIInterface = TRHIInterface<RefCounted>;
	using ArenaRHIInterface = TRHIInterface<ArenaRefCounted>;
}
