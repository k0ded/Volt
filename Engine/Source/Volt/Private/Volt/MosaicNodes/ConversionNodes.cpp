#include "vtpch.h"
#include "Volt/MosaicNodes/ConversionNodes.h"

#include <Mosaic/NodeRegistry.h>

namespace Volt::MosaicNodes
{
	REGISTER_NODE_TEMPLATE(ConversionMakeFloat3, (MakeVec3Node<float, 0.f, Mosaic::ValueBaseType::Float, "{D7F668BB-AB49-431F-BD4A-A1CA4C623EF2}"_guid>));
	REGISTER_NODE_TEMPLATE(ConversionMakeFloat4, (MakeVec4Node<float, 0.f, Mosaic::ValueBaseType::Float, "{FC283A78-CF51-4651-9835-ACFE527FD07E}"_guid>));
}
