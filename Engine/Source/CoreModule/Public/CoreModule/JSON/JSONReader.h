#include "CoreModule/Config.h"
#include "CoreModule/JSON/JSONInclude.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Concepts.h>
#include <CoreUtilities/String/VoltString.h>
#include <CoreUtilities/Filesystem/Path.h>

class VTC_API JSONReader
{
public:
	JSONReader();

	bool Parse(StringView string);

	bool OpenObject(StringView key);
	void CloseObject();

	template<typename T> bool TryGet(StringView key, T& outValue);
	template<Enum T> bool TryGet(StringView key, T& outValue);

	template<> bool TryGet(StringView key, Filesystem::Path& outValue);
	
	template<typename T> void Get(T& outValue);
	template<typename Func> void IterateArray(StringView key, Func&& func);

private:
	const nlohmann::json* GetCurrentObject() const;
	std::string GetKey(StringView key) const;

	bool m_isIteratingArray;

	const nlohmann::json* m_arrayElement = nullptr;
	nlohmann::json m_document;
	Vector<const nlohmann::json*> m_objectStack;
};

template<typename T>
bool JSONReader::TryGet(StringView key, T& outValue)
{
	const nlohmann::json* obj = GetCurrentObject();
	if (!obj)
	{
		return false;
	}

	std::string tempKey = GetKey(key);
	auto it = obj->find(tempKey);
	if (it == obj->end())
	{
		return false;
	}

	if constexpr (std::is_same_v<String, T>)
	{
		it->get_to(tempKey);
		outValue = String(tempKey.c_str(), tempKey.size());
	}
	else
	{
		it->get_to(outValue);
	}

	return true;
}

template<Enum T>
bool JSONReader::TryGet(StringView key, T& outValue)
{
	using Underlying = std::underlying_type_t<T>;

	Underlying temp{};
	if (!TryGet(key, temp))
	{
		return false;
	}

	outValue = static_cast<T>(temp);
	return true;
}

template<>
bool JSONReader::TryGet(StringView key, Filesystem::Path& outValue)
{
	String temp;
	if (!TryGet(key, temp))
	{
		return false;
	}

	outValue = Filesystem::Path(temp);
	return true;
}

template<typename T>
void JSONReader::Get(T& outValue)
{
	VT_ENSURE(m_isIteratingArray);
	VT_ENSURE(m_arrayElement != nullptr);

	if constexpr (std::is_same_v<String, T>)
	{
		std::string tempValue;
		m_arrayElement->get_to(tempValue);
		outValue = String(tempValue.c_str(), tempValue.size());
	}
	else
	{
		m_arrayElement->get_to(outValue);
	}
}

template<typename Func>
void JSONReader::IterateArray(StringView key, Func&& func)
{
	const nlohmann::json* current = GetCurrentObject();

	auto it = current->find(GetKey(key));
	if (it == current->end() || !it->is_array())
	{
		return;
	}

	m_isIteratingArray = true;

	for (const auto& elem : *it)
	{
		m_arrayElement = &elem;

		if (elem.is_object())
		{
			m_objectStack.push_back(&elem);
			func();
			m_objectStack.pop_back();
		}
		else
		{
			func();
		}
	}

	m_arrayElement = nullptr;
	m_isIteratingArray = false;
}
