#pragma once

#include "CoreUtilities/Delegates/DelegateHandle.h"
#include "CoreUtilities/VoltAssert.h"

#include <tuple>

namespace Volt
{
	template<typename ReturnType, typename... ParamTypes>
	class DelegateInstance;

	template <typename FnType, typename FunctorType, typename... VarTypes>
	class FunctorDelegateInstance;

	template <typename FnType, typename... VarTypes>
	class StaticDelegateInstance;

	template <bool Const, class UserClass, typename FnType, typename... VarTypes>
	class RawFunctionDelegateInstance;

	template<typename ReturnType, typename... ParamTypes>
	class DelegateInstance<ReturnType(ParamTypes...)>
	{
	public:
		template<typename... InParamTypes>
		explicit DelegateInstance(InParamTypes&&... params)
			//: m_paramTypes(std::forward<InParamTypes>(params)...)
		{
		}

		virtual bool IsSafeToExecute() const = 0;
		virtual ReturnType Execute(ParamTypes...) const = 0;
		virtual bool ExecuteIfSafe(ParamTypes...) const = 0;

		virtual DelegateInstance<ReturnType(ParamTypes...)>* CreateCopy() const = 0;

		VT_NODISCARD VT_INLINE DelegateHandle GetHandle() const { return m_handle; }

	protected:
		//std::tuple<ParamTypes...> m_paramTypes;

		DelegateHandle m_handle;
	};


	template <typename ReturnType, typename... ParamTypes, typename FunctorType, typename... VarTypes>
	class FunctorDelegateInstance<ReturnType(ParamTypes...), FunctorType, VarTypes...> : public DelegateInstance<ReturnType(ParamTypes...)>
	{
		static_assert(std::is_same_v<FunctorType, typename std::remove_reference_t<FunctorType>>, "FunctorType cannot be a reference");

	public:
		template <typename InFunctorType, typename... InVarTypes>
		explicit FunctorDelegateInstance(InFunctorType&& inFunctor, InVarTypes&&... vars)
			: m_functor(std::forward<InFunctorType>(inFunctor))
			, m_vars(std::forward<InVarTypes>(vars)...)
		{
		}

		bool IsSafeToExecute() const override final
		{
			//functors are always considered safe to execute
			return true;
		}

		ReturnType Execute(ParamTypes... params) const override final
		{
			return ExecuteImpl(std::make_index_sequence<sizeof...(VarTypes)>{}, std::forward<ParamTypes>(params)...);
		}

		bool ExecuteIfSafe(ParamTypes...params) const override final
		{
			//functors are always considered safe to execute
			ExecuteImpl(std::make_index_sequence<sizeof...(VarTypes)>{}, std::forward<ParamTypes>(params)...);

			return true;
		}

		DelegateInstance<ReturnType(ParamTypes...)>* CreateCopy() const override
		{
			return new FunctorDelegateInstance(*this);
		}
	private:
		template<size_t... Indices>
		ReturnType ExecuteImpl(std::index_sequence<Indices...>, ParamTypes... params) const
		{
			return m_functor(std::forward<ParamTypes>(params)..., std::get<Indices>(m_vars)...);
		}

		mutable std::remove_const_t<FunctorType> m_functor;
		std::tuple<VarTypes...> m_vars;
	};

	template <typename ReturnType, typename... ParamTypes, typename... VarTypes>
	class StaticDelegateInstance<ReturnType(ParamTypes...), VarTypes...> : public DelegateInstance<ReturnType(ParamTypes...)>
	{
		using FnPtr = ReturnType(*)(ParamTypes..., VarTypes...);


	public:
		template <typename... InVarTypes>
		explicit StaticDelegateInstance(FnPtr functionPtr, InVarTypes&&... vars)
			: m_staticFunctionPtr(functionPtr)
			, m_vars(std::forward<InVarTypes>(vars)...)
		{
			VT_ASSERT(m_staticFunctionPtr != nullptr);
		}

		bool IsSafeToExecute() const override final
		{
			//static functions are always safe to execute
			return true;
		}


		ReturnType Execute(ParamTypes... params) const override final
		{
			VT_ASSERT(m_staticFunctionPtr != nullptr);

			return ExecuteImpl(std::make_index_sequence<sizeof...(VarTypes)>{}, std::forward<ParamTypes>(params)...);
		}

		bool ExecuteIfSafe(ParamTypes... params) const override final
		{
			VT_ASSERT(m_staticFunctionPtr != nullptr);

			ExecuteImpl(std::make_index_sequence<sizeof...(VarTypes)>{}, std::forward<ParamTypes>(params)...);

			return true;
		}

		DelegateInstance<ReturnType(ParamTypes...)>* CreateCopy() const override
		{
			return new StaticDelegateInstance(*this);
		}
	private:
		template<size_t... Indices>
		ReturnType ExecuteImpl(std::index_sequence<Indices...>, ParamTypes... params) const
		{
			return m_staticFunctionPtr(std::forward<ParamTypes>(params)..., std::get<Indices>(m_vars)...);
		}

		FnPtr m_staticFunctionPtr;
		std::tuple<VarTypes...> m_vars;
	};


	template <bool Const, typename Class, typename FnType>
	struct MemFnPtrType;

	template <typename Class, typename ReturnType, typename... ParamTypes>
	struct MemFnPtrType < false, Class, ReturnType(ParamTypes...)>
	{
		typedef ReturnType(Class::* Type)(ParamTypes...);
	};

	template <typename Class, typename ReturnType, typename... ParamTypes>
	struct MemFnPtrType < true, Class, ReturnType(ParamTypes...)>
	{
		typedef ReturnType(Class::* Type)(ParamTypes...) const;
	};

	template <bool Const, class UserClass, typename ReturnType, typename... ParamTypes, typename... VarTypes >
	class RawFunctionDelegateInstance<Const, UserClass, ReturnType(ParamTypes...), VarTypes...> : public DelegateInstance<ReturnType(ParamTypes...)>
	{
		using FnPtr = typename MemFnPtrType<Const, UserClass, ReturnType(ParamTypes..., VarTypes...)>::Type;
		using UserClassPtr = std::conditional_t<Const, const UserClass*, UserClass*>;

	public:
		template <typename... InVarTypes>
		explicit RawFunctionDelegateInstance(UserClassPtr userObject, FnPtr functionPtr, InVarTypes&&... vars)
			: m_userObject(userObject)
			, m_functionPtr(functionPtr)
			, m_vars(std::forward<InVarTypes>(vars)...)
		{
			VT_ASSERT(userObject != nullptr && functionPtr != nullptr);
		}

		bool IsSafeToExecute() const override final
		{
			//Impossible to know if it's safe to execute, but trust the user here
			return true;
		}

		ReturnType Execute(ParamTypes... params) const override final
		{
			return ExecuteImpl(std::make_index_sequence<sizeof...(VarTypes)>{}, std::forward<ParamTypes>(params)...);
		}

		bool ExecuteIfSafe(ParamTypes...params) const override final
		{
			ExecuteImpl(std::make_index_sequence<sizeof...(VarTypes)>{}, std::forward<ParamTypes>(params)...);
			return true;
		}

		DelegateInstance<ReturnType(ParamTypes...)>* CreateCopy() const override
		{
			return new RawFunctionDelegateInstance(*this);
		}
	private:
		template<size_t... Indices>
		ReturnType ExecuteImpl(std::index_sequence<Indices...>, ParamTypes... params) const
		{
			using MutableUserClass = std::remove_const_t<UserClass>;

			MutableUserClass* mutableUserObject = const_cast<MutableUserClass*>(m_userObject);
			return ((*mutableUserObject).*m_functionPtr)(std::forward<ParamTypes>(params)..., std::get<Indices>(m_vars)...);
		}

		UserClassPtr m_userObject;
		FnPtr m_functionPtr;
		std::tuple<VarTypes...> m_vars;
	};
}
