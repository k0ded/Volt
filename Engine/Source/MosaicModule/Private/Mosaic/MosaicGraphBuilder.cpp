#include "mcpch.h"

#include "Mosaic/MosaicGraphBuilder.h"

namespace Mosaic
{
	MosaicGraphBuilder::MosaicGraphBuilder(MosaicGraph& graph)
		: m_graph(graph)
	{
		// Start with a clean slate.
		m_graph.Clear();
	}

	void MosaicGraphBuilder::LinkNodeParameters(UUID64 fromNode, UUID64 toNode, const std::string& fromParamName, const std::string& toParamName)
	{
		auto& underlyingGraph = m_graph.GetUnderlyingGraph();

		const auto& fromNodeData = underlyingGraph.GetNodeFromID(fromNode);
		const auto& toNodeData = underlyingGraph.GetNodeFromID(toNode);

		VT_ENSURE(fromNodeData.IsValid() && toNodeData.IsValid());

		UUID64 fromParamId = 0;
		UUID64 toParamId = 0;

		uint32_t fromParamIndex = 0;
		uint32_t toParamIndex = 0;

		for (const Parameter& parameter : fromNodeData.nodeData->GetOutputParameters())
		{
			if (parameter.name == fromParamName)
			{
				fromParamId = parameter.id;
				fromParamIndex = parameter.index;
				break;
			}
		}

		for (const Parameter& parameter : toNodeData.nodeData->GetInputParameters())
		{
			if (parameter.name == toParamName)
			{
				toParamId = parameter.id;
				toParamIndex = parameter.index;
				break;
			}
		}

		VT_ENSURE(fromParamId != 0 && toParamId != 0);

		underlyingGraph.LinkNodes(fromNode, toNode, CreateRef<MosaicEdge>(toParamIndex, fromParamIndex));
	}
}
