#include "nodepch.h"
#include "NodeGraphBase.h"

NodeGraphBase::NodeGraphBase()
{
}

NodeInstanceID NodeGraphBase::SpawnNodeOfType(VoltGUID typeID)
{
	VT_ENSURE_MSG(NodeTypeRegistry::Get().TypeExists(typeID), "Tried to spawn a node type that doesnt exist!");

	const NodeInstanceID newInstanceID = m_nextNodeInstanceID++;
	NodeInstance instance;
	instance.instanceID = newInstanceID;
	instance.position = { 0,0 };// info.position = ScreenToWorldPos(ImGui::GetMousePos());
	instance.typeGUID = typeID;
	m_nodeInstances.insert({ instance.instanceID, std::move(instance) });

	return newInstanceID;
}

void NodeGraphBase::DeleteNode(NodeInstanceID instanceID)
{
	for (int32_t i = static_cast<int32_t>(m_nodeConnections.size() - 1); i >= 0; i--)
	{
		const NodeConnection& connection = m_nodeConnections[i];
		if (connection.from == instanceID || connection.to == instanceID)
		{
			m_nodeConnections.erase(m_nodeConnections.begin() + i);
		}
	}

	m_nodeInstances.erase(instanceID);
}

