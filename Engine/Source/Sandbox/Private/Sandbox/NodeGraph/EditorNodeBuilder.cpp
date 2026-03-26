#include "sbpch.h"
#include "Sandbox/NodeGraph/EditorNodeBuilder.h"

EditorNodeTypeDefinitionBuilder::EditorNodeTypeDefinitionBuilder()
{}

void EditorNodeTypeDefinitionBuilder::Pin(PinDirection pinDirection, EditorNodePinID pinID, const char* pinName, InlineVector<uint8_t, PIN_USERDATA_MAX_SIZE>* userData, PinType pinType)
{
	EditorNodePinDefinition definition;
	memset(&definition, 0, sizeof(EditorNodePinDefinition));

	definition.direction = pinDirection;
	definition.pinID = pinID;
	definition.pinName = String(pinName);
	if (userData)
	{
		definition.userData.resize(userData->size());
		memcpy_s(definition.userData.data(), definition.userData.size(), userData->data(), userData->size());
	}
	definition.pinType = pinType;

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
