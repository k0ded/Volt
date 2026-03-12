#pragma once

#include "NodeGraph/PinType.h"

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Containers/VectorVariants.h>

#include <string>

enum class PinDirection
{
	Input,
	Output
};

// should be created by converting 4 chars, example: 'cond'
typedef uint32_t NodePinID;


struct NodePinDefinition
{
	PinDirection direction;

	NodePinID pinID;
	std::string pinName;

	PinTypeCustomDataVector pinTypeData;
	VoltGUID pinTypeGUID;
	// todo: kind of ends up being duplicated data here since many pins can use the same pin type...
	int32_t pinTypeStorageTypeSize;

	std::function<void(void*)> constructPinStorageFn;
	std::function<void(void*)> destructPinStorageFn;
};

struct NodeTypeDefinition
{
	Map<NodePinID, NodePinDefinition> inputPins;
	Map<NodePinID, NodePinDefinition> outputPins;
	bool usesDynamicPins = false;
};

class NodeTypeDefinitionBuilder
{
public:
	NodeTypeDefinitionBuilder();
	~NodeTypeDefinitionBuilder() = default;

	VT_INLINE bool UsesDynamicPins() const { return m_nodeTypeDefinition.usesDynamicPins; }
	VT_INLINE void MarkUsesDynamicPins() { m_nodeTypeDefinition.usesDynamicPins = true; }

	template<typename PinType>
	void Pin(PinDirection pinDirection, NodePinID pinID, const char* pinName, const PinType::CustomDataType& inCustomPinTypeData)
	{
		PinTypeCustomDataVector customPinTypeData;
		customPinTypeData.resize(sizeof(typename PinType::CustomDataType));
		memcpy_s(customPinTypeData.data(), customPinTypeData.size(), &inCustomPinTypeData, sizeof(typename PinType::CustomDataType));

		NodePinDefinition definition;
		memset(&definition, 0, sizeof(NodePinDefinition));

		definition.direction = pinDirection;
		definition.pinID = pinID;
		definition.pinName = std::string(pinName);
		definition.pinTypeData = std::move(customPinTypeData);
		definition.pinTypeGUID = PinType::GetStaticTypeGUID();
		definition.pinTypeStorageTypeSize = sizeof(PinType::StorageType);

		definition.constructPinStorageFn = [](void* ptr)
		{
			new(ptr)PinType::StorageType();
		};

		definition.destructPinStorageFn = [](void* ptr)
		{
			using StorageType = typename PinType::StorageType;
			StorageType* storage = reinterpret_cast<StorageType*>(ptr);
			storage->~StorageType();
		};

		switch (pinDirection)
		{
			case PinDirection::Input:
				m_nodeTypeDefinition.inputPins.insert({ pinID, definition });
				break;
			case PinDirection::Output:
				m_nodeTypeDefinition.outputPins.insert({ pinID, definition });
				break;
			default:
				break;
		}

		
	}
	template<typename PinType>
	void Pin(PinDirection pinDirection, NodePinID pinID, const char* pinName)
	{
		static_assert(typeid(PinTypeNoneCustomData) == typeid(typename PinType::CustomDataType) && "Please provide the specified custom data form pin type.");

		typename PinType::CustomDataType pinTypeCustomData;
		Pin<PinType>(pinDirection, pinID, pinName, pinTypeCustomData);
	}


	NodeTypeDefinition&& MoveDefinition() { return std::move(m_nodeTypeDefinition); }
private:

	NodeTypeDefinition m_nodeTypeDefinition;
};
