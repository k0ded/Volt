#pragma once

#include "CoreUtilities/Config.h"
#include "CoreUtilities/Core.h"
#include "CoreUtilities/Pointers/Weak.h"
#include "CoreUtilities/String/StringUtility.h"
#include "CoreUtilities/Containers/Map.h"

enum class ConsoleVariableFlags
{
	None = 0,
	ReadOnly = BIT(0)
};
VT_SETUP_ENUM_CLASS_OPERATORS(ConsoleVariableFlags);

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
	RegisteredConsoleVariable(const String& variableName, const T& defaultValue, ConsoleVariableFlags flags, StringView description);
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
	ConsoleVariableFlags m_flags;
};

template<ValidConsoleVariableType T>
class ConsoleVariable
{
public:
	ConsoleVariable(StringView variableName, const T& defaultValue, StringView description);
	ConsoleVariable(StringView variableName, const T& defaultValue, ConsoleVariableFlags flags, StringView description);

	VT_INLINE const T& GetValue() const { return *reinterpret_cast<const T*>(m_variableReference.Lock()->Get()); }
	VT_INLINE void SetValue(const T& value) { m_variableReference.Lock()->Set(&value); }

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

	VT_INLINE const T& GetValue() const { return *reinterpret_cast<T*>(m_variableReference.Lock()->Get()); }
	VT_INLINE void SetValue(const T& value) { m_variableReference.Lock()->Set(&value); }

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
	static Weak<RegisteredConsoleVariable<T>> RegisterVariable(StringView variableName, const T& defaultValue, ConsoleVariableFlags flags, StringView description);

	template<ValidConsoleVariableType T>
	static Weak<RegisteredConsoleVariable<T>> FindVariable(const String& variableName);

	VTCOREUTIL_API static Weak<RegisteredConsoleVariableBase> GetVariable(const String& variableName);
	VTCOREUTIL_API static bool VariableExists(const String& variableName);

	static Map<String, Ref<RegisteredConsoleVariableBase>>& GetRegisteredVariables();

	VTCOREUTIL_API static ConsoleVariableRegistry& Get();

private:
	Map<String, Ref<RegisteredConsoleVariableBase>> m_registeredVariables;
};

#include "CoreUtilities/ConsoleVariableRegistry.inl"
