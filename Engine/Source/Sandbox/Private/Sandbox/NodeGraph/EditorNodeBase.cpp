#include "sbpch.h"
#include "Sandbox/NodeGraph/EditorNodeBase.h"
#include "Sandbox/NodeGraph/EditorNodeBuilder.h"

void NothingNode::Build(EditorNodeBuilder& nodeBuilder)
{}


enum class VisualScriptPinType : uint8_t
{
	Flow,
	Adaptive,
	Bool,
	Float,
	Int,
	String,
	Asset
};
struct VisualScriptPinData
{
	VisualScriptPinType pinType;

};
void NothingNode::MakeTypeDefinition(EditorNodeTypeDefinitionBuilder& builder)
{
	EditorPinUserData flowUserData;
	flowUserData.push_back(1); // 1 for flow

	EditorPinUserData boolVariableUserData;
	boolVariableUserData.push_back(0); // 0 for not flow
	boolVariableUserData.push_back(3); // 3 is the variable type ID for bool


	builder.Pin(PinDirection::Input, 'in', "In", &flowUserData);
	builder.Pin(PinDirection::Input, 'cond', "Condition", &boolVariableUserData, PinType::Custom); // custom to show a checkbox if it isnt connected
	builder.Pin(PinDirection::Input, 'paud', "Pauda", &boolVariableUserData, PinType::Custom); // custom to show a checkbox if it isnt connected

	builder.Pin(PinDirection::Output, 'true', "True", &flowUserData);
	builder.Pin(PinDirection::Output, 'fals', "False", &flowUserData);

	//builder.MarkUsesDynamicPins();
}
