#include "vtcorepch.h"

#include "Volt-Core/Console/ConsoleVariableRegistry.h"

namespace Volt
{
	ConsoleVariableRegistry::ConsoleVariableRegistry()
	{
	}

	ConsoleVariableRegistry::~ConsoleVariableRegistry()
	{
	}

	std::unordered_map<std::string, Ref<RegisteredConsoleVariableBase>>& ConsoleVariableRegistry::GetRegisteredVariables()
	{
		return ConsoleVariableRegistry::Get().m_registeredVariables;
	}

	ConsoleVariableRegistry& ConsoleVariableRegistry::Get()
	{
		static ConsoleVariableRegistry registry;
		return registry;
	}

	bool ConsoleVariableRegistry::VariableExists(const std::string& variableName)
	{
		std::string tempVarName = ::Utility::ToLower(std::string(variableName));
		return ConsoleVariableRegistry::Get().m_registeredVariables.contains(tempVarName);
	}

	Weak<RegisteredConsoleVariableBase> ConsoleVariableRegistry::GetVariable(const std::string& variableName)
	{
		const std::string tempVarName = ::Utility::ToLower(variableName);
		return ConsoleVariableRegistry::Get().m_registeredVariables.at(tempVarName);
	}
}
