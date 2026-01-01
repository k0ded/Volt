#include "CoreUtilities/Containers/Vector.h"
#include "CoreUtilities/Config.h"
#include "CoreUtilities/Concepts.h"
#include "CoreUtilities/JSON/JSONInclude.h"

class VTCOREUTIL_API JSONReader
{
public:
	JSONReader();

	bool Parse(std::string_view string);

	bool OpenObject(std::string_view key);
	void CloseObject();

	template<typename T> bool TryGet(std::string_view key, T& outValue);
	template<Enum T> bool TryGet(std::string_view key, T& outValue);

	template<> bool TryGet(std::string_view key, std::filesystem::path& outValue);
	
	template<typename T> void Get(T& outValue);
	template<typename Func> void IterateArray(std::string_view key, Func&& func);

private:
	const nlohmann::json* GetCurrentObject() const;

	bool m_isIteratingArray;

	const nlohmann::json* m_arrayElement = nullptr;
	nlohmann::json m_document;
	Vector<const nlohmann::json*> m_objectStack;
};

template<typename T>
bool JSONReader::TryGet(std::string_view key, T& outValue)
{
	const nlohmann::json* obj = GetCurrentObject();
	if (!obj)
	{
		return false;
	}

	auto it = obj->find(key);
	if (it == obj->end())
	{
		return false;
	}

	it->get_to(outValue);
	return true;
}

template<Enum T>
bool JSONReader::TryGet(std::string_view key, T& outValue)
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
bool JSONReader::TryGet(std::string_view key, std::filesystem::path& outValue)
{
	std::string temp;
	if (!TryGet(key, temp))
	{
		return false;
	}

	outValue = std::filesystem::path(temp);
	return true;
}

template<typename T>
void JSONReader::Get(T& outValue)
{
	VT_ENSURE(m_isIteratingArray);
	VT_ENSURE(m_arrayElement != nullptr);

	m_arrayElement->get_to(outValue);
}

template<typename Func>
void JSONReader::IterateArray(std::string_view key, Func&& func)
{
	const nlohmann::json* current = GetCurrentObject();

	auto it = current->find(key);
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
