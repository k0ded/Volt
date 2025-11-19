#include "sbpch.h"
#include "Sandbox/NodeGraph/EditorNodeGraph.h"

NodeInstanceID EditorNodeGraph::SpawnNodeOfType(VoltGUID typeID)
{
	if (!m_registeredNodeTypes.contains(typeID))
	{
		VT_ENSURE_MSG(m_registeredNodeTypes.contains(typeID), "Tried to spawn a node type that doesnt exist!");
		return 0;
	}

	NodeTypeInfo& typeInfo = m_registeredNodeTypes[typeID];
	Ref<EditorNodeTypeBase> newNode = typeInfo.CreateInstanceFunc();
	m_spawnedNodes.push_back(newNode);

	//todo: return proper instance ID
	return 1;
}
