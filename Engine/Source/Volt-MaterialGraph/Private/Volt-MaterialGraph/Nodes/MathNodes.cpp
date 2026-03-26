#include "vtmgpch.h"
#include "Volt-MaterialGraph/Nodes/MathNodes.h"

#include <Mosaic/MosaicGraph.h>
#include <Mosaic/MosaicHelpers.h>
#include <Mosaic/NodeRegistry.h>
#include <Mosaic/MosaicShaderWriter.h>

namespace Volt::MosaicNodes
{
	AddNode::AddNode(Mosaic::MosaicGraph* ownerGraph)
		: Mosaic::MosaicNode(ownerGraph)
	{
		AddInputParameter("A", Mosaic::ValueBaseType::Dynamic, 1, false);
		AddInputParameter("B", Mosaic::ValueBaseType::Dynamic, 1, false);

		AddOutputParameter("", Mosaic::ValueBaseType::Dynamic, 1, false);
	}

	const Mosaic::ResultInfo AddNode::Compile(const GraphNode<Ref<class Mosaic::MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const
	{
		constexpr const char* nodeStr = "const {0} {1} = {2} + {3}; \n";
		constexpr Mosaic::TypeInfo DEFAULT_PARAM_TYPEINFO{ Mosaic::ValueBaseType::Float, 1 };

		String A = FormatString("{}", GetInputParameter(0).Get<float>());
		String B = FormatString("{}", GetInputParameter(1).Get<float>());

		Mosaic::TypeInfo AInfo = DEFAULT_PARAM_TYPEINFO;
		Mosaic::TypeInfo BInfo = DEFAULT_PARAM_TYPEINFO;

		for (const auto& edgeId : underlyingNode.GetInputEdges())
		{
			const auto edge = underlyingNode.GetEdgeFromID(edgeId);
			const uint32_t paramIndex = edge.metaDataType->GetParameterInputIndex();
		
			const auto& node = underlyingNode.GetNodeFromID(edge.startNode);
			const Mosaic::ResultInfo info = node.nodeData->Compile(node, edge.metaDataType->GetParameterOutputIndex(), shaderWriter);

			if (paramIndex == 0)
			{
				A = info.resultParamName;
				AInfo = info.resultType;
			}
			else if (paramIndex == 1)
			{
				B = info.resultParamName;
				BInfo = info.resultType;
			}
		}

		const String varName = m_graph->GetNextVariableName();
		
		const Mosaic::TypeInfo resultType = Mosaic::Helpers::GetPromotedTypeInfo(AInfo, BInfo);

		String codeBlock = FormatString(nodeStr, Mosaic::Helpers::GetTypeNameFromTypeInfo(resultType), varName, A, B);
		shaderWriter.AppendCodeBlock(codeBlock);

		Mosaic::ResultInfo resultInfo{};
		resultInfo.resultParamName = varName;
		resultInfo.resultType = resultType;

		return resultInfo;
	}

	REGISTER_NODE(AddNode);

	MultiplyNode::MultiplyNode(Mosaic::MosaicGraph* ownerGraph)
		: Mosaic::MosaicNode(ownerGraph)
	{
		AddInputParameter("A", Mosaic::ValueBaseType::Dynamic, 1, false);
		AddInputParameter("B", Mosaic::ValueBaseType::Dynamic, 1, false);

		AddOutputParameter("", Mosaic::ValueBaseType::Dynamic, 1, false);
	}
	
	const Mosaic::ResultInfo MultiplyNode::Compile(const GraphNode<Ref<class Mosaic::MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const
	{
		constexpr const char* nodeStr = "const {0} {1} = {2} * {3}; \n";
		constexpr Mosaic::TypeInfo DEFAULT_PARAM_TYPEINFO{ Mosaic::ValueBaseType::Float, 1 };

		String A = FormatString("{}", GetInputParameter(0).Get<float>());
		String B = FormatString("{}", GetInputParameter(1).Get<float>());

		Mosaic::TypeInfo AInfo = DEFAULT_PARAM_TYPEINFO;
		Mosaic::TypeInfo BInfo = DEFAULT_PARAM_TYPEINFO;

		for (const auto& edgeId : underlyingNode.GetInputEdges())
		{
			const auto edge = underlyingNode.GetEdgeFromID(edgeId);
			const uint32_t paramIndex = edge.metaDataType->GetParameterInputIndex();

			const auto& node = underlyingNode.GetNodeFromID(edge.startNode);
			const Mosaic::ResultInfo info = node.nodeData->Compile(node, edge.metaDataType->GetParameterOutputIndex(), shaderWriter);

			if (paramIndex == 0)
			{
				A = info.resultParamName;
				AInfo = info.resultType;
			}
			else if (paramIndex == 1)
			{
				B = info.resultParamName;
				BInfo = info.resultType;
			}
		}

		const String varName = m_graph->GetNextVariableName();

		const Mosaic::TypeInfo resultType = Mosaic::Helpers::GetPromotedTypeInfo(AInfo, BInfo);

		String codeBlock = FormatString(nodeStr, Mosaic::Helpers::GetTypeNameFromTypeInfo(resultType), varName, A, B);
		shaderWriter.AppendCodeBlock(codeBlock);

		Mosaic::ResultInfo resultInfo{};
		resultInfo.resultParamName = varName;
		resultInfo.resultType = resultType;

		return resultInfo;
	}

	REGISTER_NODE(MultiplyNode);
}
