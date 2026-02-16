#include "nodepch.h"

#include "NodeGraph/NodeTypeBase.h"
#include "NodeGraph/NodeTypeDefinitionBuilder.h"

REGISTER_NODE_TYPE(NothingNode);
void NothingNode::MakeTypeDefinition(NodeTypeDefinitionBuilder& builder)
{
	//inputs
	builder.Pin<PinType_Flow>(PinDirection::Input, 'in', "In");
	builder.Pin<PinType_Bool>(PinDirection::Input, 'cond', "Condition"); // custom to show a checkbox if it isnt connected

	PinType_FloatCustomData floatData;
	floatData.isSlider = true;
	floatData.minBound = 0;
	floatData.maxBound = 10;
	builder.Pin<PinType_Float>(PinDirection::Input, 'flt', "In", floatData);


	//outputs
	builder.Pin<PinType_Flow>(PinDirection::Output, 'true', "True");
	builder.Pin<PinType_Flow>(PinDirection::Output, 'fals', "False");

	//builder.MarkUsesDynamicPins();
}

NodeTypeRegistry& NodeTypeRegistry::Get()
{
	static NodeTypeRegistry registry;
	return registry;
}

