#pragma once

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Containers/Vector.h>

#include <string>
#include <functional>
#include <map>

struct EditorNodeTypeRegistrationInfo
{
	std::string Name = "";


	//std::function<>()
};

struct EditorNodeType
{
	typedef uint32_t IDType;

	const std::string& GetName() { return Name; }
	IDType GetTypeID() { return TypeID; }

private:
	friend class EditorNodeGraph;

	EditorNodeType(const EditorNodeTypeRegistrationInfo& regInfo, uint32_t inTypeID)
		:Name(regInfo.Name), TypeID(inTypeID)
	{}

	//name is mostly for debug purposes
	const std::string Name;
	const uint32_t TypeID;
};

typedef uint32_t EditorNodeHandle;
class EditorNodeGraph
{
public:
	EditorNodeGraph();
	~EditorNodeGraph();

	EditorNodeType::IDType RegisterNodeType(const EditorNodeTypeRegistrationInfo& regInfo);
	Ref<const EditorNodeType*> GetTypeFromID(EditorNodeType::IDType typeID);
	Ref<const EditorNodeType*> GetTypeFromNodeHandle(EditorNodeHandle nodeHandle);
	EditorNodeType::IDType GetTypeIDFromNodeHandle(EditorNodeHandle nodeHandle);

	EditorNodeHandle CreateNode(EditorNodeType::IDType typeID);

	
private:
	Vector<EditorNodeHandle> m_nodes;
	std::unordered_map<EditorNodeHandle, EditorNodeType::IDType> m_nodeToType;
	std::unordered_map<EditorNodeType::IDType, Ref<EditorNodeType>> m_registeredNodeTypes;



};
