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
	const NodeTypeDefinition& GetTypeDefinition() const
	{
		return NodeTypeRegistry::Get().GetTypeDefinition(typeGUID);
	}

	//OBS: generally recommended to use the type safe version GetPinStorage
	void* GetPinStoragePtr(int32_t pinID)
	{
		const NodeTypeDefinition& nodeTypeDefinition = GetTypeDefinition();
		const bool inputContains = nodeTypeDefinition.inputPins.contains(pinID);
		const bool outputContains = nodeTypeDefinition.outputPins.contains(pinID);
		VT_CHECK(inputContains || outputContains);
		VT_CHECK(inputContains != outputContains);

		return &((*storagePtr)[pinIDToStorageOffset[pinID]]);
	}

	template<typename PinType>
	PinType::StorageType& GetPinStorage(int32_t pinID)
	{
		const NodeTypeDefinition& nodeTypeDefinition = GetTypeDefinition();
		const bool inputContains = nodeTypeDefinition.inputPins.contains(pinID);
		const bool outputContains = nodeTypeDefinition.outputPins.contains(pinID);
		VT_CHECK(inputContains || outputContains);
		VT_CHECK(inputContains != outputContains);

		const Map<NodePinID, NodePinDefinition>& targetPinsMap = (inputContains ? nodeTypeDefinition.inputPins : nodeTypeDefinition.outputPins);
		const NodePinDefinition& pinTypeDefinition = targetPinsMap.at(pinID);
		VT_CHECK(pinTypeDefinition.pinTypeGUID == PinType::GetStaticGUID());

		return reinterpret_cast<PinType::StorageType&>(storagePtr[pinIDToStorageOffset[pinID]]);
	}
	template<typename PinType>
	const PinType::StorageType& GetPinStorage(int32_t pinID) const
	{
		return GetPinStorage<PinType>(pinID);
	}

	NodeInstanceID InstanceID() const { return instanceID; }

	void SetPosition(const glm::vec2& newPosition) { position = newPosition; }
	void SetDesiredSize(const glm::vec2& newDesiredSize) { desiredSize = newDesiredSize; }

	const glm::vec2& GetPosition()  const { return position; }
	const glm::vec2& GetDesiredSize()  const { return desiredSize; }


//private:
	friend class NodeGraphBase;

	Map<NodePinID, int32_t> pinIDToStorageOffset;
	Vector<uint8_t>* storagePtr;

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
	void MakeStorageSpacesForNodeInstance(NodeInstance& instance);
	//start at 1 to reserve 0 as null/invalid ID
	NodeInstanceID m_nextNodeInstanceID = 1;

	Map<NodeInstanceID, NodeInstance> m_nodeInstances;
	Vector<NodeConnection> m_nodeConnections;

	//todo: this is kind of a memory leak since we never use the space left by deleted nodes...
	Vector<uint8_t> m_storage;
};

template<typename T>
inline NodeInstanceID NodeGraphBase::SpawnNodeOfType()
{
	return SpawnNodeOfType(T::GetStaticTypeGUID());
}
