#pragma once

#include "ProjectUpgradeClient/Common/SerializationHelpers.h"
#include <CoreUtilities/Containers/Vector.h>

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <functional>

class YAMLStreamReader
{
public:
	YAMLStreamReader();
	virtual ~YAMLStreamReader() = default;

	const bool HasKey(const String& key);
	const bool IsSequenceEmpty(const String& key);

	void EnterScope(const String& key);
	void ExitScope();

	template<typename T>
	const T ReadAtKey(const String& key, const T& defaultValue);

	template<typename T>
	const T ReadValue();

	template<typename T>
	const T ReadKeyValue();

	void ForEach(const String& key, std::function<void()> function);

	inline YAML::Node& GetRawNode() { return m_currentNode; }

protected:
	YAML::Node m_currentNode;
	YAML::Node m_currentSequenceKeyNode;
	Vector<YAML::Node> m_nodeStack;

	YAML::Node m_rootNode;
};

template<typename T>
inline const T YAMLStreamReader::ReadAtKey(const String& key, const T& defaultValue)
{
	return m_currentNode[key] ? m_currentNode[key].as<T>() : defaultValue;
}

template<typename T>
inline const T YAMLStreamReader::ReadValue()
{
	return m_currentNode.as<T>();
}

template<typename T>
inline const T YAMLStreamReader::ReadKeyValue()
{
	return m_currentSequenceKeyNode.as<T>();
}
