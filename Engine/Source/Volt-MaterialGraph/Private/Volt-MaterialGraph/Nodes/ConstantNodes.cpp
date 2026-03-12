#include "vtmgpch.h"
#include "Volt-MaterialGraph/Nodes/ConstantNodes.h"


namespace Volt::MosaicNodes
{
	REGISTER_NODE(ConstantFloat);
	REGISTER_NODE(ConstantFloat2);
	REGISTER_NODE(ConstantFloat3);
	REGISTER_NODE(ConstantFloat4);

	REGISTER_NODE(ConstantInt);
	REGISTER_NODE(ConstantInt2);
	REGISTER_NODE(ConstantInt3);
	REGISTER_NODE(ConstantInt4);

	REGISTER_NODE(Color3);
	REGISTER_NODE(Color4);
}
