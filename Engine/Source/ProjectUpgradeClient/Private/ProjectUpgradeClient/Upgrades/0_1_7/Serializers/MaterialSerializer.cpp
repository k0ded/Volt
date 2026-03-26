#include "ProjectUpgradeClient/Upgrades/0_1_7/Serializers/MaterialSerializer.h"
#include "ProjectUpgradeClient/Common/YAMLMemoryStreamReader.h"
#include "ProjectUpgradeClient/Common/YAMLMemoryStreamWriter.h"

#define private public
#include <Volt-Assets/MaterialAsset.h>
#undef private
#include <Volt-Assets/MaterialCompilerSubSystem.h>

#include <Volt-MaterialGraph/MaterialGraph.h>
#include <Volt-Renderer/Material/RenderMaterial.h>

#include <Volt-FileSystem/Filesystem.h>

#include <AssetSystem/AssetManager.h>

#include <RenderCore/Shader/DefaultShaders.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <Mosaic/MosaicGraph.h>
#include <Mosaic/MosaicNode.h>

#include <SubSystem/SubSystemManager.h>

namespace Volt
{
	void MaterialSerializer::Serialize(const AssetMetadata_0_1_7* metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const
	{
		const AssetReference<MaterialAsset> mosaicAsset = asset.ConvertTo<MaterialAsset>();

		const auto& graph = mosaicAsset->GetMaterialGraph()->GetMosaicGraph();

		YAMLMemoryStreamWriter streamWriter{};
		streamWriter.BeginMap();
		streamWriter.BeginMapNamned("MosaicGraph");
		streamWriter.SetKey("guid", mosaicAsset->m_graph->m_materialGUID);
		streamWriter.SetKey("state", graph.GetEditorState());

		streamWriter.BeginSequence("Nodes");
		for (const auto& node : graph.GetUnderlyingGraph().GetNodes())
		{
			streamWriter.BeginMap();
			streamWriter.SetKey("id", node.id);
			streamWriter.SetKey("guid", node.nodeData->GetGUID());
			streamWriter.SetKey("state", node.nodeData->GetEditorState());

			streamWriter.BeginMapNamned("custom");
#if 0
			node.nodeData->SerializeCustom(streamWriter);
#endif
			streamWriter.EndMap();

			streamWriter.BeginSequence("inputParams");
			for (const auto& input : node.nodeData->GetInputParameters())
			{
				streamWriter.BeginMap();
				streamWriter.SetKey("index", input.index);
#if 0
				if (input.serializationFunc)
				{
					input.serializationFunc(streamWriter, input);
				}
#endif
				streamWriter.EndMap();
			}
			streamWriter.EndSequence();

			streamWriter.BeginSequence("outputParams");
			for (const auto& output : node.nodeData->GetOutputParameters())
			{
				streamWriter.BeginMap();
				streamWriter.SetKey("index", output.index);

#if 0
				if (output.serializationFunc)
				{
					output.serializationFunc(streamWriter, output);
				}
#endif

				streamWriter.EndMap();
			}
			streamWriter.EndSequence();

			streamWriter.EndMap();
		}
		streamWriter.EndSequence();

		streamWriter.BeginSequence("Edges");
		for (const auto& edge : graph.GetUnderlyingGraph().GetEdges())
		{
			streamWriter.BeginMap();
			streamWriter.SetKey("id", edge.id);
			streamWriter.SetKey("startNode", edge.startNode);
			streamWriter.SetKey("endNode", edge.endNode);
			streamWriter.SetKey("inputIndex", edge.metaDataType->GetParameterInputIndex());
			streamWriter.SetKey("outputIndex", edge.metaDataType->GetParameterOutputIndex());
			streamWriter.EndMap();
		}
		streamWriter.EndSequence();

		streamWriter.EndMap();
		streamWriter.EndMap();

		BinaryStreamWriter binaryStreamWriter{};
		const size_t compressedDataOffset = AssetSerializer::WriteMetadata(*metadata, asset->GetVersion(), binaryStreamWriter);

		auto buffer = streamWriter.WriteAndGetBuffer();
		binaryStreamWriter.Write(buffer);
		buffer.Release();

		const auto filePath = g_assetManager->GetAssetFilesystemPath(metadata->filepath);
		binaryStreamWriter.WriteToDisk(filePath, true, compressedDataOffset);
	}

	template<typename T>
	void ReadDataFromBuffer(const DataBuffer& buffer, T& outData)
	{
		memcpy_s(&outData, sizeof(T), buffer.As<void>(), buffer.GetSize());
	}

	bool MaterialSerializer::Deserialize(const AssetMetadata_0_1_7* metadata, AssetReference<Asset> destinationAsset) const
	{
		const auto filePath = g_assetManager->GetAssetFilesystemPath(metadata->filepath);

		AssetReference<MaterialAsset> materialAsset = destinationAsset.ConvertTo<MaterialAsset>();

		if (!Filesystem::Exists(filePath))
		{
			VT_LOG(Error, "File {0} not found!", metadata->filepath);
			materialAsset->SetFlag(AssetFlag::Missing, true);
			return false;
		}

		BinaryStreamReader binaryStreamReader{ filePath };

		if (!binaryStreamReader.IsStreamValid())
		{
			VT_LOG(Error, "Failed to open file: {0}!", metadata->filepath);
			materialAsset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		SerializedAssetMetadata serializedMetadata = AssetSerializer::ReadMetadata(binaryStreamReader);
		VT_ASSERT_MSG(serializedMetadata.version == materialAsset->GetVersion(), "Incompatible version!");

		DataBuffer buffer{};
		binaryStreamReader.Read(buffer);

		YAMLMemoryStreamReader streamReader{};
		if (!streamReader.ConsumeBuffer(buffer))
		{
			VT_LOG(Error, "Failed to read file {0}!", metadata->filepath);
			materialAsset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		materialAsset->m_graph = CreateRef<MaterialGraph>();
		materialAsset->m_renderMaterial = CreateRef<RenderMaterial>(String(materialAsset->GetAssetName()));
		materialAsset->m_graph->m_graph->Clear();

		streamReader.EnterScope("MosaicGraph");

		materialAsset->m_graph->m_materialGUID = streamReader.ReadAtKey("guid", VoltGUID::Null());
		materialAsset->m_graph->m_graph->GetEditorState() = streamReader.ReadAtKey("state", String(""));

		auto& underlyingGraph = materialAsset->m_graph->m_graph->GetUnderlyingGraph();

		streamReader.ForEach("Nodes", [&]()
		{
			const UUID64 nodeId = streamReader.ReadAtKey("id", UUID64(0));
			const VoltGUID guid = streamReader.ReadAtKey("guid", VoltGUID::Null());
			const String state = streamReader.ReadAtKey("state", String());

			materialAsset->m_graph->m_graph->AddNode(nodeId, guid);
			auto& node = underlyingGraph.GetNodeFromID(nodeId);

			node.nodeData->GetEditorState() = state;

#if 0
			if (streamReader.HasKey("custom"))
			{
				streamReader.EnterScope("custom");
				node.nodeData->DeserializeCustom(streamReader);
				streamReader.ExitScope();
			}

			streamReader.ForEach("inputParams", [&]()
			{
				const uint32_t index = streamReader.ReadAtKey("index", 0u);
				auto& param = node.nodeData->GetInputParameter(index);

				if (param.deserializationFunc)
				{
					param.deserializationFunc(streamReader, param);
				}
			});
			
			streamReader.ForEach("outputParams", [&]()
			{
				const uint32_t index = streamReader.ReadAtKey("index", 0u);
				auto& param = node.nodeData->GetOutputParameter(index);

				if (param.deserializationFunc)
				{
					param.deserializationFunc(streamReader, param);
				}
			});
#endif
		});

		streamReader.ForEach("Edges", [&]()
		{
			const UUID64 edgeId = streamReader.ReadAtKey("id", UUID64(0));
			const UUID64 startNodeId = streamReader.ReadAtKey("startNode", UUID64(0));
			const UUID64 endNodeId = streamReader.ReadAtKey("endNode", UUID64(0));
			const uint32_t inputIndex = streamReader.ReadAtKey("inputIndex", uint32_t(0));
			const uint32_t outputIndex = streamReader.ReadAtKey("outputIndex", uint32_t(0));

			underlyingGraph.LinkNodes(edgeId, startNodeId, endNodeId, CreateRef<Mosaic::MosaicEdge>(inputIndex, outputIndex));
		});

		streamReader.ExitScope();

		String logStr = FormatString("Loaded material {0} with textures: \n", (uint64_t)metadata->handle);

		// #TODO_Ivar: This should probably happen automatically while deserializing the texture nodes
		for (const auto tex : materialAsset->m_graph->GetTextureHandles())
		{
			logStr += FormatString("		- {0}\n", (uint64_t)tex);
		}

		if (MaterialCompilerSubSystem* compilerSubSystem = SubSystemManager::GetSubSystem<MaterialCompilerSubSystem>(); compilerSubSystem != nullptr)
		{
			compilerSubSystem->RequestMaterialCompilation(materialAsset);
		}

		VT_LOG(Trace, logStr);
		return true;
	}
}
