#pragma once

template<ValidConsoleVariableType T>
inline RegisteredConsoleVariable<T>::RegisteredConsoleVariable(const String& variableName, const T& defaultValue, ConsoleVariableFlags flags, StringView description)
	: m_value(defaultValue), 
	m_variableName(variableName), 
	m_description(description),
	m_flags(flags)
{}

template<ValidConsoleVariableType T>
inline const void* RegisteredConsoleVariable<T>::Get() const
{
	return reinterpret_cast<const void*>(&m_value);
}

template<ValidConsoleVariableType T>
inline void RegisteredConsoleVariable<T>::Set(const void* value)
{
	if ((m_flags & ConsoleVariableFlags::ReadOnly) != ConsoleVariableFlags::None)
	{
		return;
	}

	m_value = *reinterpret_cast<const T*>(value);
}


template<ValidConsoleVariableType T>
void RegisteredConsoleVariable<T>::SetOverride(const void* value)
{
	m_value = *reinterpret_cast<const T*>(value);
}

template<ValidConsoleVariableType T>
inline ConsoleVariable<T>::ConsoleVariable(StringView variableName, const T& defaultValue, StringView description)
{
	m_variableReference = ConsoleVariableRegistry::RegisterVariable<T>(variableName, defaultValue, ConsoleVariableFlags::None, description);
}

template<ValidConsoleVariableType T>
ConsoleVariable<T>::ConsoleVariable(StringView variableName, const T& defaultValue, ConsoleVariableFlags flags, StringView description)
{
	m_variableReference = ConsoleVariableRegistry::RegisterVariable<T>(variableName, defaultValue, flags, description);
}

template<ValidConsoleVariableType T>
inline Weak<RegisteredConsoleVariable<T>> ConsoleVariableRegistry::RegisterVariable(StringView variableName, const T& defaultValue, ConsoleVariableFlags flags, StringView description)
{
	String tempVarName = ::Utility::ToLower(String(variableName));

	Ref<RegisteredConsoleVariable<T>> consoleVariable = CreateRef<RegisteredConsoleVariable<T>>(tempVarName, defaultValue, flags, description);

	VT_ASSERT_MSG(!ConsoleVariableRegistry::Get().m_registeredVariables.contains(tempVarName), "Command variable with name already registered!");
	ConsoleVariableRegistry::Get().m_registeredVariables[tempVarName] = consoleVariable;

	return consoleVariable;
}

template<ValidConsoleVariableType T>
inline Weak<RegisteredConsoleVariable<T>> ConsoleVariableRegistry::FindVariable(const String& variableName)
{
	const String tempVarName = ::Utility::ToLower(variableName);

	if (ConsoleVariableRegistry::Get().m_registeredVariables.contains(tempVarName))
	{
		return ConsoleVariableRegistry::Get().m_registeredVariables.at(tempVarName);
	}

	return Weak<RegisteredConsoleVariable<T>>();
}

template<ValidConsoleVariableType T>
inline ConsoleVariableRef<T>::ConsoleVariableRef(StringView variableName)
{
	m_variableReference = ConsoleVariableRegistry::FindVariable(variableName);
	VT_ASSERT_MSG(m_variableReference, "Variable with name not found!");
}
