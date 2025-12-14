#include "sbpch.h"
#include "Sandbox/NodeGraph/EditorNodeBuilder.h"

EditorNodeTypeDefinitionBuilder::EditorNodeTypeDefinitionBuilder()
	: m_usesDynamicPins(false)
{}

void EditorNodeTypeDefinitionBuilder::Pin(PinDirection pinDirection, EditorNodePinID pinID, std::string_view pinName, InlineVector<uint8_t, EditorNodePinDefinition::PIN_USERDATA_MAX_SIZE>* userData, PinType pinType)
{
	EditorNodePinDefinition definition;
	memset(&definition, 0, sizeof(EditorNodePinDefinition));

	definition.direction = pinDirection;
	definition.pinID = pinID;
	definition.pinName = pinName;
	if (userData)
	{
		definition.userData.resize(userData->size());
		memcpy_s(definition.userData.data(), definition.userData.size(), userData->data(), userData->size());
	}
	definition.pinType = pinType;

	switch (pinDirection)
	{
		case PinDirection::Input:
			m_inputPins.insert({ pinID, definition });
			break;
		case PinDirection::Output:
			m_outputPins.insert({ pinID, definition });
			break;
		default:
			break;
	}
}
