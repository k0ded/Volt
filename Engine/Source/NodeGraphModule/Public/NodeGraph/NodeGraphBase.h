#pragma once

#include "NodeGraph/Config.h"
#include "NodeGraph/NodeTypeBase.h"
#include "NodeGraph/NodeTypeDefinitionBuilder.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Core.h>
#include <CoreUtilities/VoltGUID.h>
#include <CoreUtilities/Containers/Map.h>

#include <cstdint> 

typedef uint32_t NodeInstanceID;

struct NodeInstance
{
	NodeInstanceID instanceID;

	glm::vec2 position;
	glm::vec2 desiredSize = { 60, 40 };
	VoltGUID typeGUID;
};
struct NodeConnection
{
	NodeInstanceID from;
	NodeInstanceID to;
};

class VTNODE_API NodeGraphBase
{
public:
	NodeGraphBase();
	~NodeGraphBase() = default;

	template<typename T>
	NodeInstanceID SpawnNodeOfType();
	NodeInstanceID SpawnNodeOfType(VoltGUID typeID);

	void DeleteNode(NodeInstanceID instanceID);

	const Map<NodeInstanceID, NodeInstance>& GetNodeInstancesMap() { return m_nodeInstances; }

	const NodeInstance& GetNodeInstance(NodeInstanceID instanceID) { return m_nodeInstances[instanceID]; }
	NodeInstance& GetMutableNodeInstance(NodeInstanceID instanceID) { return m_nodeInstances[instanceID]; }
protected:

	//start at 1 to reserve 0 as null/invalid ID
	NodeInstanceID m_nextNodeInstanceID = 1;

	Map<NodeInstanceID, NodeInstance> m_nodeInstances;
	Vector<NodeConnection> m_nodeConnections;
};

template<typename T>
inline NodeInstanceID NodeGraphBase::SpawnNodeOfType()
{
	return SpawnNodeOfType(T::GetStaticTypeGUID());
}
