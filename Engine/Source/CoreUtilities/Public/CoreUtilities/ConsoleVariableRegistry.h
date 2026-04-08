#pragma once

#include "CoreUtilities/Config.h"
#include "CoreUtilities/Pointers/Weak.h"
#include "CoreUtilities/String/StringUtility.h"
#include "CoreUtilities/Containers/Map.h"

namespace Volt
{
	class RegisteredConsoleVariableBase
	{
	public:
		virtual ~RegisteredConsoleVariableBase() = default;
		virtual const void* Get() const = 0;
		virtual void Set(const void* value) = 0;

		virtual StringView GetName() const = 0;
		virtual StringView GetDescription() const = 0;
		
		virtual bool IsInteger() const = 0;
		virtual bool IsFloat() const = 0;
		virtual bool IsString() const = 0;
	};

	template<typename T>
	concept ValidConsoleVariableType = std::is_same_v<T, int32_t> || std::is_same_v<T, float> || std::is_same_v<T, String>;

	template<ValidConsoleVariableType T>
	class RegisteredConsoleVariable : public RegisteredConsoleVariableBase
	{
	public:
		RegisteredConsoleVariable(const String& variableName, const T& defaultValue, StringView description);
		~RegisteredConsoleVariable() override = default;

		VT_NODISCARD const void* Get() const override;
		void Set(const void* value) override;

		VT_NODISCARD inline StringView GetName() const override { return m_variableName; }
		VT_NODISCARD inline StringView GetDescription() const override { return m_description; }

		VT_NODISCARD inline constexpr bool IsInteger() const override { return std::is_integral_v<T>; }
		VT_NODISCARD inline constexpr bool IsFloat() const override { return std::is_floating_point_v<T>; }
		VT_NODISCARD inline constexpr bool IsString() const override { return std::is_same_v<T, String>; }

	private:
		T m_value;
		String m_variableName;
		StringView m_description;
	};

	template<ValidConsoleVariableType T>
	class ConsoleVariable
	{
	public:
		ConsoleVariable(StringView variableName, const T& defaultValue, StringView description);

		const T& GetValue() const { return *reinterpret_cast<const T*>(m_variableReference.Lock()->Get()); }
		void SetValue(const T& value) { m_variableReference.Lock()->Set(&value); }

		T& operator=(const T& other)
		{
			if (this == &other)
			{
				return GetValue();
			}

			SetValue(other);
			return GetValue();
		}

	private:
		Weak<RegisteredConsoleVariable<T>> m_variableReference;
	};

	template<ValidConsoleVariableType T>
	class ConsoleVariableRef
	{
	public:
		ConsoleVariableRef(StringView variableName);

		const T& GetValue() const { return *reinterpret_cast<T*>(m_variableReference.Lock()->Get()); }
		void SetValue(const T& value) { m_variableReference.Lock()->Set(&value); }

		T& operator=(const T& other)
		{
			if (this == &other)
			{
				return GetValue();
			}

			SetValue(other);
		}

	private:
		Weak<RegisteredConsoleVariable<T>> m_variableReference;
	};

	class ConsoleVariableRegistry
	{
	public:
		ConsoleVariableRegistry();
		~ConsoleVariableRegistry();

		template<ValidConsoleVariableType T>
		static Weak<RegisteredConsoleVariable<T>> RegisterVariable(StringView variableName, const T& defaultValue, StringView description);

		template<ValidConsoleVariableType T>
		static Weak<RegisteredConsoleVariable<T>> FindVariable(const String& variableName);

		VTCOREUTIL_API static Weak<RegisteredConsoleVariableBase> GetVariable(const String& variableName);
		VTCOREUTIL_API static bool VariableExists(const String& variableName);

		static Map<String, Ref<RegisteredConsoleVariableBase>>& GetRegisteredVariables();

		VTCOREUTIL_API static ConsoleVariableRegistry& Get();

	private:
		Map<String, Ref<RegisteredConsoleVariableBase>> m_registeredVariables;
	};

	template<ValidConsoleVariableType T>
	inline RegisteredConsoleVariable<T>::RegisteredConsoleVariable(const String& variableName, const T& defaultValue, StringView description)
		: m_value(defaultValue), m_variableName(variableName), m_description(description)
	{
	}

	template<ValidConsoleVariableType T>
	inline const void* RegisteredConsoleVariable<T>::Get() const
	{
		return reinterpret_cast<const void*>(&m_value);
	}

	template<ValidConsoleVariableType T>
	inline void RegisteredConsoleVariable<T>::Set(const void* value)
	{
		m_value = *reinterpret_cast<const T*>(value);
	}

	template<ValidConsoleVariableType T>
	inline ConsoleVariable<T>::ConsoleVariable(StringView variableName, const T& defaultValue, StringView description)
	{
		m_variableReference = ConsoleVariableRegistry::RegisterVariable<T>(variableName, defaultValue, description);
	}

	template<ValidConsoleVariableType T>
	inline Weak<RegisteredConsoleVariable<T>> ConsoleVariableRegistry::RegisterVariable(StringView variableName, const T& defaultValue, StringView description)
	{
		String tempVarName = ::Utility::ToLower(String(variableName));

		Ref<RegisteredConsoleVariable<T>> consoleVariable = CreateRef<RegisteredConsoleVariable<T>>(tempVarName, defaultValue, description);

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
}
