#include "vtcorepch.h"

#include "Volt-Core/Console/ConsoleVariableRegistry.h"

#include <CoreUtilities/String/StringUtility.h>

namespace Volt
{
	ConsoleVariableRegistry::ConsoleVariableRegistry()
	{
	}

	ConsoleVariableRegistry::~ConsoleVariableRegistry()
	{
	}

	std::unordered_map<String, Ref<RegisteredConsoleVariableBase>>& ConsoleVariableRegistry::GetRegisteredVariables()
	{
		return ConsoleVariableRegistry::Get().m_registeredVariables;
	}

	ConsoleVariableRegistry& ConsoleVariableRegistry::Get()
	{
		static ConsoleVariableRegistry registry;
		return registry;
	}

	bool ConsoleVariableRegistry::VariableExists(const String& variableName)
	{
		String tempVarName = ::Utility::ToLower(variableName);
		return ConsoleVariableRegistry::Get().m_registeredVariables.contains(tempVarName);
	}

	Weak<RegisteredConsoleVariableBase> ConsoleVariableRegistry::GetVariable(const String& variableName)
	{
		const String tempVarName = ::Utility::ToLower(variableName);
		return ConsoleVariableRegistry::Get().m_registeredVariables.at(tempVarName);
	}
}
