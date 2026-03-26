#include "vtmgpch.h"

#include "Volt-MaterialGraph/MaterialGraph.h"

#include <Volt-Platforms/Platform.h>

#include <Mosaic/MosaicGraph.h>
#include <Mosaic/MosaicNode.h>

#include <CoreUtilities/Archive/MemoryArchive.h>

namespace Volt
{
	struct SerializedParameter
	{
		uint32_t index;
		MemoryWriter parameterSerializationDataWriter;
		MemoryReader parameterSerializationDataReader;

		friend Archive& operator<<(Archive& archive, SerializedParameter& serializedParameter)
		{
			archive << serializedParameter.index;

			if (archive.IsLoading())
			{
				archive << serializedParameter.parameterSerializationDataReader;
			}
			else
			{
				archive << serializedParameter.parameterSerializationDataWriter;
			}

			return archive;
		}
	};

	struct SerializedNode
	{
		UUID64 id;
		VoltGUID nodeTypeGUID;
		String editorState;
	
		MemoryWriter customSerializationDataWriter;
		MemoryReader customSerializationDataReader;
	
		Vector<SerializedParameter> inputParameters;
		Vector<SerializedParameter> outputParameters;

		friend Archive& operator<<(Archive& archive, SerializedNode& serializedNode)
		{
			archive << serializedNode.id;
			archive << serializedNode.nodeTypeGUID;
			archive << serializedNode.editorState;
			
			if (archive.IsLoading())
			{
				archive << serializedNode.customSerializationDataReader;
			}
			else
			{
				archive << serializedNode.customSerializationDataWriter;
			}

			archive << serializedNode.inputParameters;
			archive << serializedNode.outputParameters;
			return archive;
		}
	};

	struct SerializedEdge
	{
		UUID64 id;
		UUID64 startNode;
		UUID64 endNode;
		uint32_t inputIndex;
		uint32_t outputIndex;

		friend Archive& operator<<(Archive& archive, SerializedEdge& serializedEdge)
		{
			archive << serializedEdge.id;
			archive << serializedEdge.startNode;
			archive << serializedEdge.endNode;
			archive << serializedEdge.inputIndex;
			archive << serializedEdge.outputIndex;

			return archive;
		}
	};

    MaterialGraph::MaterialGraph()
    {
		m_graph = Mosaic::MosaicGraph::CreateDefaultGraph();
		m_materialGUID = PlatformMisc::GenerateGUID();
    }

	Vector<AssetHandle> MaterialGraph::GetTextureHandles() const
	{
		return Vector<AssetHandle>();
	}

	void MaterialGraph::Serialize(Archive& archive)
	{
		archive << m_materialGUID;

		String editorState = m_graph->GetEditorState();
		archive << editorState;

		if (archive.IsLoading())
		{
			m_graph->Clear();
			m_graph->GetEditorState() = editorState;
		}

		Vector<SerializedNode> serializedNodes;

		if (!archive.IsLoading())
		{
			for (const auto& node : m_graph->GetUnderlyingGraph().GetNodes())
			{
				auto& serializedNode = serializedNodes.emplace_back();
				serializedNode.id = node.id;
				serializedNode.nodeTypeGUID = node.nodeData->GetGUID();
				serializedNode.editorState = node.nodeData->GetEditorState();
				node.nodeData->SerializeCustom(serializedNode.customSerializationDataWriter);
				serializedNode.customSerializationDataWriter.Close();
			
				for (auto& input : node.nodeData->GetInputParameters())
				{
					auto& serializedInput = serializedNode.inputParameters.emplace_back();
					serializedInput.index = input.index;
					if (input.serializationFunc)
					{
						input.serializationFunc(serializedInput.parameterSerializationDataWriter, input);
					}
					serializedInput.parameterSerializationDataWriter.Close();
				}

				for (auto& output : node.nodeData->GetOutputParameters())
				{
					auto& serializedOutput = serializedNode.outputParameters.emplace_back();
					serializedOutput.index = output.index;
					if (output.serializationFunc)
					{
						output.serializationFunc(serializedOutput.parameterSerializationDataWriter, output);
					}
					serializedOutput.parameterSerializationDataWriter.Close();
				}
			}
		}

		archive << serializedNodes;

		Vector<SerializedEdge> serializedEdges;

		if (!archive.IsLoading())
		{
			for (const auto& edge : m_graph->GetUnderlyingGraph().GetEdges())
			{
				auto& serializedEdge = serializedEdges.emplace_back();
				serializedEdge.id = edge.id;
				serializedEdge.startNode = edge.startNode;
				serializedEdge.endNode = edge.endNode;
				serializedEdge.inputIndex = edge.metaDataType->GetParameterInputIndex();
				serializedEdge.outputIndex = edge.metaDataType->GetParameterOutputIndex();
			}
		}

		archive << serializedEdges;

		// Loading graph
		if (archive.IsLoading())
		{
			auto& underlyingGraph = m_graph->GetUnderlyingGraph();

			for (SerializedNode& serializedNode : serializedNodes)
			{
				m_graph->AddNode(serializedNode.id, serializedNode.nodeTypeGUID);
				auto& node = underlyingGraph.GetNodeFromID(serializedNode.id);

				node.nodeData->GetEditorState() = serializedNode.editorState;
				node.nodeData->SerializeCustom(serializedNode.customSerializationDataReader);

				for (SerializedParameter& serializedInput : serializedNode.inputParameters)
				{
					auto& param = node.nodeData->GetInputParameter(serializedInput.index);
					if (param.serializationFunc)
					{
						param.serializationFunc(serializedInput.parameterSerializationDataReader, param);
					}
				}

				for (SerializedParameter& serializedOutput : serializedNode.outputParameters)
				{
					auto& param = node.nodeData->GetOutputParameter(serializedOutput.index);
					if (param.serializationFunc)
					{
						param.serializationFunc(serializedOutput.parameterSerializationDataReader, param);
					}
				}
			}

			for (const SerializedEdge& serializedEdge : serializedEdges)
			{
				underlyingGraph.LinkNodes(serializedEdge.id, serializedEdge.startNode, serializedEdge.endNode, CreateRef<Mosaic::MosaicEdge>(serializedEdge.inputIndex, serializedEdge.outputIndex));
			}
		}
	}
}
