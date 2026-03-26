#include "vtmgpch.h"
#include "Volt-MaterialGraph/Nodes/PBROutputNode.h"

#include <Mosaic/NodeRegistry.h>
#include <Mosaic/MosaicShaderWriter.h>

#include <CoreUtilities/FormatterExtension.h>

#include <glm/glm.hpp>

namespace Volt::MosaicNodes
{
	PBROutputNode::PBROutputNode(Mosaic::MosaicGraph* ownerGraph)
		: Mosaic::MosaicNode(ownerGraph)
	{
		AddInputParameter("Base Color", Mosaic::ValueBaseType::Float, 4, glm::vec4(1.f, 1.f, 1.f, 1.f), false);
		AddInputParameter("Metallic", Mosaic::ValueBaseType::Float, 1, 0.f, false);
		AddInputParameter("Roughness", Mosaic::ValueBaseType::Float, 1, 0.9f, false);
		AddInputParameter("Normal", Mosaic::ValueBaseType::Float, 3, glm::vec3(0.f, 0.f, 1.f), false);
		AddInputParameter("Emissive", Mosaic::ValueBaseType::Float, 3, glm::vec3(0.f), false);
	}

	const Mosaic::ResultInfo PBROutputNode::Compile(const GraphNode<Ref<class Mosaic::MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const
	{
		constexpr const char* nodeStr = "EvaluatedMaterial materialResult;\n"
										"materialResult.Setup();\n"
										"materialResult.albedo = {0};\n"
										"materialResult.metallic = {1};\n"
										"materialResult.roughness = {2};\n"
										"materialResult.normal = {3};\n"
										"materialResult.emissive = {4};\n\n"
			
										"return materialResult;\n";

		String paramStrings[5];

		paramStrings[0] = FormatString("{}", GetInputParameter(0).Get<glm::vec4>());
		paramStrings[1] = FormatString("{}", GetInputParameter(1).Get<float>());
		paramStrings[2] = FormatString("{}", GetInputParameter(2).Get<float>());
		paramStrings[3] = FormatString("{}", GetInputParameter(3).Get<glm::vec3>());
		paramStrings[4] = FormatString("{}", GetInputParameter(4).Get<glm::vec3>());

		for (const auto& edgeId : underlyingNode.GetInputEdges())
		{
			const auto& edge = underlyingNode.GetEdgeFromID(edgeId);
			const uint32_t paramIndex = edge.metaDataType->GetParameterInputIndex();

			const auto& node = underlyingNode.GetNodeFromID(edge.startNode);
			const Mosaic::ResultInfo info = node.nodeData->Compile(node, edge.metaDataType->GetParameterOutputIndex(), shaderWriter);
		
			paramStrings[paramIndex] = info.resultParamName;
		}

		String result = FormatString(nodeStr, paramStrings[0], paramStrings[1], paramStrings[2], paramStrings[3], paramStrings[4]);
		shaderWriter.AppendCodeBlock(result);

		Mosaic::ResultInfo resultInfo{};
		resultInfo.resultParamName = "";

		return resultInfo;
	}

	REGISTER_NODE(PBROutputNode);
}
