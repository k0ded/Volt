#include "vtmgpch.h"
#include "Volt-MaterialGraph/Nodes/Normal/DeriveNormalZNode.h"

#include <Mosaic/MosaicGraph.h>
#include <Mosaic/NodeRegistry.h>

namespace Volt::MosaicNodes
{
	REGISTER_NODE(DeriveNormalZNode);

	DeriveNormalZNode::DeriveNormalZNode(Mosaic::MosaicGraph* ownerGraph)
		: Mosaic::MosaicNode(ownerGraph)
	{
		AddInputParameter("XY", Mosaic::ValueBaseType::Float, 2, glm::vec2(0.f), false);
		AddOutputParameter("Result", Mosaic::ValueBaseType::Float, 3, glm::vec3(0.f), false);
	}

	const std::string DeriveNormalZNode::GetName() const
	{
		return "DeriveNormalZ";
	}

	const std::string DeriveNormalZNode::GetCategory() const
	{
		return "Utility";
	}

	const glm::vec4 DeriveNormalZNode::GetColor() const
	{
		return 1.f;
	}

	const Mosaic::ResultInfo DeriveNormalZNode::Compile(const GraphNode<Ref<class MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const
	{
		constexpr const char* nodeStr = "const float3 {} = float3({}, sqrt(1.f - saturate({}.x * {}.x + {}.y * {}.y)));\n";

		std::string xyVector = std::format("{}", GetInputParameter(0).Get<glm::vec2>());

		for (const auto& edgeId : underlyingNode.GetInputEdges())
		{
			const auto& edge = underlyingNode.GetEdgeFromID(edgeId);

			const auto& node = underlyingNode.GetNodeFromID(edge.startNode);
			const Mosaic::ResultInfo info = node.nodeData->Compile(node, edge.metaDataType->GetParameterOutputIndex(), shaderWriter);

			xyVector = info.resultParamName;
		}

		const std::string varName = m_graph->GetNextVariableName();
		const std::string result = std::format(nodeStr, varName, xyVector, xyVector, xyVector, xyVector, xyVector);
		shaderWriter.AppendCodeBlock(result);

		Mosaic::ResultInfo resultInfo{};
		resultInfo.resultParamName = varName;
		resultInfo.resultType.baseType = Mosaic::ValueBaseType::Float;
		resultInfo.resultType.vectorSize = 3;

		return resultInfo;
	}
}
