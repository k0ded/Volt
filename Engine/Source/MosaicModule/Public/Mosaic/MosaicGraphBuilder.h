#pragma once

#include "Mosaic/Config.h"
#include "Mosaic/MosaicGraph.h"
#include "Mosaic/MosaicNode.h"
#include "Mosaic/NodeRegistry.h"
#include "Mosaic/Parameter.h"

namespace Mosaic
{
	class MosaicGraphBuilder
	{
	public:
		VTMOSAIC_API MosaicGraphBuilder(MosaicGraph& graph);

		template<typename NodeType>
		UUID64 AddNode()
		{
			auto& underlyingGraph = m_graph.GetUnderlyingGraph();
			return underlyingGraph.AddNode(NodeRegistry::Get().CreateNode(NodeType::GetStaticGUID(), &m_graph));
		}

		template<typename NodeType>
		NodeType& GetNodeAsType(UUID64 nodeId)
		{
			auto& underlyingGraph = m_graph.GetUnderlyingGraph();
			return *std::static_pointer_cast<NodeType>(underlyingGraph.GetNodeFromID(nodeId).nodeData);
		}

		template<typename ParamDataType>
		void SetNodeParameterData(UUID64 nodeId, const std::string& parameterName, const ParamDataType& data)
		{
			auto& underlyingGraph = m_graph.GetUnderlyingGraph();
			auto& node = underlyingGraph.GetNodeFromID(nodeId);

			for (Parameter& parameter : node.nodeData->GetOutputParameters())
			{
				if (parameter.name == parameterName)
				{
					parameter.Get<ParamDataType>() = data;
					break;
				}
			}
		}

		VTMOSAIC_API void LinkNodeParameters(UUID64 fromNode, UUID64 toNode, const std::string& fromParamName, const std::string& toParamName);

	private:
		MosaicGraph& m_graph;
	};
}
