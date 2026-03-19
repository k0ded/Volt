#pragma once

#include "EntitySystem/Config.h"
#include "EntitySystem/Scripting/ECSEnvironmentStorage.h"

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Pointers/Unique.h>

class ECSEventDispatcher;

class VTES_API ScriptingEngine
{
public:
	ScriptingEngine();
	~ScriptingEngine();

	void OnRuntimeStart();
	void OnRuntimeEnd();

	template<typename T>
	const T& GetECSEnvironmentOfType() const
	{
		constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();
		const void* dataPtr = GetECSEnvironmentOfTypeInternal(typeIndex);

		VT_ENSURE_MSG(dataPtr != nullptr, "Type was not found! Are you sure it has been registered?");

		const T& data = *reinterpret_cast<const T*>(dataPtr);
		return data;
	}

	template<typename T>
	T& GetECSEnvironmentOfType()
	{
		constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();
		void* dataPtr = GetECSEnvironmentOfTypeInternal(typeIndex);

		VT_ENSURE_MSG(dataPtr != nullptr, "Type was not found! Are you sure it has been registered?");

		T& data = *reinterpret_cast<T*>(dataPtr);
		return data;
	}

	ECSEventDispatcher& GetEventDispatcher() { return *m_ecsEventDispatcher; }

private:
	const void* GetECSEnvironmentOfTypeInternal(TypeTraits::TypeIndex typeIndex) const;
	void* GetECSEnvironmentOfTypeInternal(TypeTraits::TypeIndex typeIndex);

	ECSEnvironmentStorage m_ecsEnvironmentStorage;
	Unique<ECSEventDispatcher> m_ecsEventDispatcher;
};
