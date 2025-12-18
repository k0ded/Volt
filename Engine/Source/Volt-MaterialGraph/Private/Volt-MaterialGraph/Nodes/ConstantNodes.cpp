#include "vtmgpch.h"
#include "Volt-MaterialGraph/Nodes/ConstantNodes.h"


namespace Volt::MosaicNodes
{
	REGISTER_NODE_TEMPLATE(ConstantFloat);
	REGISTER_NODE_TEMPLATE(ConstantFloat2);
	REGISTER_NODE_TEMPLATE(ConstantFloat3);
	REGISTER_NODE_TEMPLATE(ConstantFloat4);

	REGISTER_NODE_TEMPLATE(ConstantInt);
	REGISTER_NODE_TEMPLATE(ConstantInt2);
	REGISTER_NODE_TEMPLATE(ConstantInt3);
	REGISTER_NODE_TEMPLATE(ConstantInt4);

	REGISTER_NODE_TEMPLATE(Color3);
	REGISTER_NODE_TEMPLATE(Color4);
}
