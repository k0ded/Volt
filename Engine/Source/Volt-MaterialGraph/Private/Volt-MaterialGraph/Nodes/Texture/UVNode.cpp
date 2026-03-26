#include "vtmgpch.h"

#include "Volt-MaterialGraph/Nodes/Texture/UVNode.h"

#include <Mosaic/MosaicGraph.h>
#include <Mosaic/NodeRegistry.h>
#include <Mosaic/MosaicShaderWriter.h>

namespace Volt::MosaicNodes
{
	UVNode::UVNode(Mosaic::MosaicGraph* ownerGraph)
		: Mosaic::MosaicNode(ownerGraph)
	{
		AddOutputParameter("UV", Mosaic::ValueBaseType::Float, 2, false);
		AddOutputParameter("U", Mosaic::ValueBaseType::Float, 1, false);
		AddOutputParameter("V", Mosaic::ValueBaseType::Float, 1, false);
	}

	UVNode::~UVNode()
	{
	}

	void UVNode::Reset()
	{
		m_evaluated = false;
	}

	static void GetCorrectedColorVariableName(Mosaic::ResultInfo& resultInfo, uint32_t outputIndex, uint32_t vectorSize)
	{
		if (outputIndex == 0)
		{
			resultInfo.resultType.vectorSize = vectorSize;
		}
		else if (outputIndex == 1)
		{
			resultInfo.resultParamName += ".r";
		}
		else if (outputIndex == 2)
		{
			resultInfo.resultParamName += ".g";
		}
	}

	const Mosaic::ResultInfo UVNode::Compile(const GraphNode<Ref<class Mosaic::MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const
	{
		constexpr const char* nodeStr = "const float2 {0} = evalData.texCoords; \n";

		if (m_evaluated)
		{
			Mosaic::ResultInfo resultInfo{};
			resultInfo.resultParamName = m_evaluatedVariableName;
			resultInfo.resultType = Mosaic::TypeInfo{ Mosaic::ValueBaseType::Float, 1 };

			GetCorrectedColorVariableName(resultInfo, outputIndex, 2);

			return resultInfo;
		}

		const String varName = m_graph->GetNextVariableName();

		String result = FormatString(nodeStr, varName);
		shaderWriter.AppendCodeBlock(result);

		Mosaic::ResultInfo resultInfo{};
		resultInfo.resultParamName = varName;
		resultInfo.resultType = Mosaic::TypeInfo{ Mosaic::ValueBaseType::Float, 2 };

		m_evaluatedVariableName = varName;
		m_evaluated = true;

		GetCorrectedColorVariableName(resultInfo, outputIndex, 2);

		return resultInfo;
	}

	REGISTER_NODE(UVNode);
}
