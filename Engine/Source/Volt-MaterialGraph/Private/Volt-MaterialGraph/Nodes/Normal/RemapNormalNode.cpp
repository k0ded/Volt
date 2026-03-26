#include "vtmgpch.h"

#include "Volt-MaterialGraph/Nodes/Normal/RemapNormalNode.h"

#include <Mosaic/MosaicGraph.h>
#include <Mosaic/NodeRegistry.h>

namespace Volt::MosaicNodes
{
	REGISTER_NODE(RemapNormalNode);

	RemapNormalNode::RemapNormalNode(Mosaic::MosaicGraph* ownerGraph)
		: MosaicNode(ownerGraph)
	{
		AddInputParameter("Normal", Mosaic::ValueBaseType::Float, 3, glm::vec3(0.f), false);
		AddOutputParameter("Remapped Normal", Mosaic::ValueBaseType::Float, 3, glm::vec3(0.f), false);
	}

	const String RemapNormalNode::GetName() const
	{
		return "RemapNormal";
	}
	
	const String RemapNormalNode::GetCategory() const
	{
		return "Utility";
	}
	
	const glm::vec4 RemapNormalNode::GetColor() const
	{
		return 1.f;
	}
	
	const Mosaic::ResultInfo RemapNormalNode::Compile(const GraphNode<Ref<class MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const
	{
		constexpr const char* nodeStr = "const float3 {} = {} * 2.f - 1.f;\n";

		String normal = FormatString("{}", GetInputParameter(0).Get<glm::vec3>());

		for (const auto& edgeId : underlyingNode.GetInputEdges())
		{
			const auto& edge = underlyingNode.GetEdgeFromID(edgeId);
			const uint32_t paramIndex = edge.metaDataType->GetParameterInputIndex();

			const auto& node = underlyingNode.GetNodeFromID(edge.startNode);
			const Mosaic::ResultInfo info = node.nodeData->Compile(node, edge.metaDataType->GetParameterOutputIndex(), shaderWriter);

			if (paramIndex == 0)
			{
				normal = info.resultParamName;
			}
		}

		const String varName = m_graph->GetNextVariableName();
		const String result = FormatString(nodeStr, varName, normal);
		shaderWriter.AppendCodeBlock(result);

		Mosaic::ResultInfo resultInfo{};
		resultInfo.resultParamName = varName;
		resultInfo.resultType.baseType = Mosaic::ValueBaseType::Float;
		resultInfo.resultType.vectorSize = 3;

		return resultInfo;
	}
}
