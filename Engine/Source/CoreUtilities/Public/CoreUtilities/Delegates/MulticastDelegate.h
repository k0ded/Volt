#pragma once

#include "CoreUtilities/Delegates/DelegateInstance.h"

#include <CoreUtilities/Containers/Vector.h>

namespace Volt
{
	template<typename ReturnType, typename... ParamTypes>
	class MulticastDelegate
	{};

	template<typename ReturnType, typename... ParamTypes>
	class MulticastDelegate<ReturnType(ParamTypes...)>
	{
		static_assert(sizeof(ReturnType) == 0, "The return type of multicast delegates must be void!");
	};

	template<typename... ParamTypes>
	class MulticastDelegate<void(ParamTypes...)>
	{
	private:
		using FnType = void(ParamTypes...);

	public:
		using ReturnType = void;

		template <typename... VarTypes>
		using FnPtr = ReturnType(*)(ParamTypes..., VarTypes...);

		template <typename UserClass, typename... VarTypes>
		using MemberFnPtr = typename MemFnPtrType<false, UserClass, ReturnType(ParamTypes..., std::decay_t<VarTypes>...)>::Type;

		template <typename UserClass, typename... VarTypes>
		using ConstMemberFnPtr = typename MemFnPtrType<true, UserClass, ReturnType(ParamTypes..., std::decay_t<VarTypes>...)>::Type;

		MulticastDelegate() = default;

		MulticastDelegate(const MulticastDelegate& other)
		{
			// #TODO_Ivar: Copy
		}

		MulticastDelegate(MulticastDelegate&& other) noexcept
			: m_delegateInstances(std::move(other.m_delegateInstances))
		{}

		~MulticastDelegate()
		{}

		MulticastDelegate& operator=(const MulticastDelegate& other)
		{
			// #TODO_Ivar: Copy
			if (this != &other)
			{

			}
			return *this;
		}

		MulticastDelegate& operator=(MulticastDelegate&& other)
		{
			if (this != &other)
			{
				m_delegateInstances = std::move(other.m_delegateInstances);
			}
			return *this;
		}

		void Broadcast(ParamTypes... params) const
		{
			for (DelegateInstance<FnType>* instance : m_delegateInstances)
			{
				instance->ExecuteIfSafe(std::forward<ParamTypes>(params)...);
			}
		}

		template<typename FunctorType, typename... LambdaParamTypes>
		DelegateHandle AddLambda(FunctorType&& functor, LambdaParamTypes&&... params)
		{
			DelegateInstance<FnType>* newInstance = this->template CreateDelegateInstance<FunctorDelegateInstance<FnType, typename std::remove_reference<FunctorType>::type, std::decay_t<LambdaParamTypes>...>>(std::forward<FunctorType>(functor), std::forward<LambdaParamTypes>(params)...);
			m_delegateInstances.emplace_back(newInstance);

			return newInstance->GetHandle();
		}

		template< typename... StaticFnParamTypes>
		DelegateHandle AddStatic(typename std::type_identity<ReturnType(*)(ParamTypes..., std::decay_t<StaticFnParamTypes>...)>::type func, StaticFnParamTypes&&... params)
		{
			DelegateInstance<FnType>* newInstance = this->template CreateDelegateInstance<StaticDelegateInstance<FnType, std::decay_t<StaticFnParamTypes>...>>(func, std::forward<StaticFnParamTypes>(params)...);
			m_delegateInstances.emplace_back(newInstance);

			return newInstance->GetHandle();
		}

		template <typename UserClass, typename... RawFnParamTypes>
		DelegateHandle AddRaw(UserClass* userObject, typename MemFnPtrType<false, UserClass, ReturnType(ParamTypes..., std::decay_t<RawFnParamTypes>...)>::Type func, RawFnParamTypes&&... params)
		{
			static_assert(!std::is_const_v<UserClass>, "Attempting to bind a delegate with a const object pointer and non-const member function.");

			DelegateInstance<FnType>* newInstance = this->template CreateDelegateInstance<RawFunctionDelegateInstance<false, UserClass, FnType, std::decay_t<RawFnParamTypes>...>>(userObject, func, std::forward<RawFnParamTypes>(params)...);
			m_delegateInstances.emplace_back(newInstance);

			return newInstance->GetHandle();
		}

		template <typename UserClass, typename... RawFnParamTypes>
		DelegateHandle AddRaw(const UserClass* userObject, typename MemFnPtrType<true, UserClass, ReturnType(ParamTypes..., std::decay_t<RawFnParamTypes>...)>::Type func, RawFnParamTypes&&... params)
		{
			DelegateInstance<FnType>* newInstance = this->template CreateDelegateInstance<RawFunctionDelegateInstance<true, UserClass, FnType, std::decay_t<RawFnParamTypes>...>>(userObject, func, std::forward<RawFnParamTypes>(params)...);
			m_delegateInstances.emplace_back(newInstance);

			return newInstance->GetHandle();
		}

		void Remove(DelegateHandle delegateHandle)
		{
			for (size_t i = 0; i < m_delegateInstances.size(); ++i)
			{
				if (m_delegateInstances[i]->GetHandle() == delegateHandle)
				{
					m_delegateInstances.erase_unsorted(m_delegateInstances.begin() + i);
					break;
				}
			}
		}

	private:
		template<typename DelegateInstanceType, typename... DelegateInstanceParams>
		DelegateInstanceType* CreateDelegateInstance(DelegateInstanceParams&&... params)
		{
			return new DelegateInstanceType(std::forward<DelegateInstanceParams>(params)...);
		}

		Vector<DelegateInstance<FnType>*> m_delegateInstances;
	};
}
