#include "mcpch.h"
#include "Mosaic/MosaicGraph.h"

#include "Mosaic/NodeRegistry.h"
#include "Mosaic/MosaicNode.h"
#include "Mosaic/MosaicShaderWriter.h"

#include <CoreUtilities/Profiling/Profiling.h>

namespace Mosaic
{
	MosaicGraph::MosaicGraph()
	{
	}

	MosaicGraph::~MosaicGraph()
	{
	}

	const MosaicShaderWriter MosaicGraph::Compile() const
	{
		VT_PROFILE_FUNCTION();

		constexpr VoltGUID OUTPUT_GUID = "{343B2C0A-C4E3-41BB-8629-F9939795AC76}"_guid;

		for (auto& node : m_graph.GetNodes())
		{
			node.nodeData->Reset();
		}

		UUID64 outputId = 0;
		for (const auto& node : m_graph.GetNodes())
		{
			if (node.nodeData->GetGUID() == OUTPUT_GUID)
			{
				outputId = node.id;
				break;
			}
		}

		const auto& node = m_graph.GetNodeFromID(outputId);

		if (!node.nodeData)
		{
			return {};
		}

		MosaicShaderWriter shaderWriter{};
		node.nodeData->Compile(node, 0, shaderWriter);

		return shaderWriter;
	}

	void MosaicGraph::Clear()
	{
		m_graph.Clear();
	}

	Unique<MosaicGraph> MosaicGraph::CreateDefaultGraph()
	{
		Unique<MosaicGraph> graph = CreateUnique<MosaicGraph>();

		constexpr VoltGUID Color4ConstantGUID = "{C032E7D5-D545-4DEF-8EC4-6A3980BC41B2}"_guid;
		constexpr VoltGUID PBROutputNodeGUID = "{343B2C0A-C4E3-41BB-8629-F9939795AC76}"_guid;

		auto colorConstantNode = graph->m_graph.AddNode(NodeRegistry::Get().CreateNode(Color4ConstantGUID, graph.GetRaw()));
		auto pbrOutputNode = graph->m_graph.AddNode(NodeRegistry::Get().CreateNode(PBROutputNodeGUID, graph.GetRaw()));

		graph->m_graph.LinkNodes(colorConstantNode, pbrOutputNode, CreateRef<MosaicEdge>(0, 0));

		return graph;
	}

	void MosaicGraph::AddNode(const UUID64 uuid, const VoltGUID guid)
	{
		m_graph.AddNode(uuid, NodeRegistry::Get().CreateNode(guid, this));
	}

	void MosaicGraph::AddNode(const VoltGUID guid)
	{
		m_graph.AddNode(NodeRegistry::Get().CreateNode(guid, this));
	}

	uint32_t MosaicGraph::GetNextVariableIndex()
	{
		return m_currentVariableCount++;
	}
	
	uint32_t MosaicGraph::GetNextTextureIndex()
	{
		m_textureCount++;

		if (!m_availiableTextureIndices.empty())
		{
			const uint32_t index = m_availiableTextureIndices.back();
			m_availiableTextureIndices.pop_back();

			return index;
		}

		return m_currentTextureIndex++;
	}

	const String MosaicGraph::GetNextVariableName()
	{
		return FormatString("variable{}", GetNextVariableIndex());
	}

	void MosaicGraph::ForfeitTextureIndex(uint32_t textureIndex)
	{
		m_availiableTextureIndices.emplace_back(textureIndex);
		m_textureCount--;
	}
}
