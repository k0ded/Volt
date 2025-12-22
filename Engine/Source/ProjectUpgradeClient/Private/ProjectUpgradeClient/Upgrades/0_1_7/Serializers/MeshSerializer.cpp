#include "ProjectUpgradeClient/Upgrades/0_1_7/Serializers/MeshSerializer.h"
#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializationCommon.h"

#define private public
#include <Volt-Assets/MeshAsset.h>
#undef private

#include <Volt-Renderer/Mesh/Mesh.h>

#include <AssetSystem/AssetManager.h>

namespace Volt
{
	struct MeshSerializationData_V2
	{
		Vector<glm::vec3> vertexPositions;
		Vector<VertexMaterialData> vertexMaterialData;
		Vector<VertexAnimationData> vertexAnimationData;
		Vector<uint32_t> indices;

		Vector<AssetHandle> materials;

		glm::vec3 boundingSphereCenter;
		float boundingSphereRadius;

		Vector<SubMesh> subMeshes;

		static void Serialize(BinaryStreamWriter& streamWriter, const MeshSerializationData_V2& data)
		{
			streamWriter.WriteRaw(data.vertexPositions);
			streamWriter.WriteRaw(data.vertexMaterialData);
			streamWriter.WriteRaw(data.vertexAnimationData);
			streamWriter.WriteRaw(data.indices);
			streamWriter.WriteRaw(data.materials);
			streamWriter.Write(data.boundingSphereCenter);
			streamWriter.Write(data.boundingSphereRadius);
			streamWriter.Write(data.subMeshes);
		}

		static void Deserialize(BinaryStreamReader& streamReader, MeshSerializationData_V2& outData)
		{
			streamReader.ReadRaw(outData.vertexPositions);
			streamReader.ReadRaw(outData.vertexMaterialData);
			streamReader.ReadRaw(outData.vertexAnimationData);
			streamReader.ReadRaw(outData.indices);
			streamReader.ReadRaw(outData.materials);
			streamReader.Read(outData.boundingSphereCenter);
			streamReader.Read(outData.boundingSphereRadius);
			streamReader.Read(outData.subMeshes);
		}
	};

	void MeshSerializer::Serialize(const AssetMetadata_0_1_7* metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const
	{
		AssetReference<MeshAsset> meshAsset = asset.ConvertTo<MeshAsset>();

		BinaryStreamWriter streamWriter{};
		const size_t compressedDataOffset = AssetSerializer::WriteMetadata(*metadata, asset->GetVersion(), streamWriter);

		MeshSerializationData_V2 serializationData{};

		const auto& vertexContainer = meshAsset->m_mesh->GetVertexContainer();

		serializationData.materials = meshAsset->m_materials;
		serializationData.vertexPositions = vertexContainer.positions;
		serializationData.vertexMaterialData = vertexContainer.materialData;
		serializationData.vertexAnimationData = vertexContainer.animationData;
		serializationData.indices = meshAsset->m_mesh->GetIndices();
		serializationData.boundingSphereCenter = meshAsset->m_mesh->GetBoundingSphere().center;
		serializationData.boundingSphereRadius = meshAsset->m_mesh->GetBoundingSphere().radius;
		serializationData.subMeshes = meshAsset->m_mesh->GetSubMeshes();

		streamWriter.Write(serializationData);

		const auto filePath = g_assetManager->GetAssetFilesystemPath(metadata->filepath);
		streamWriter.WriteToDisk(filePath, true, compressedDataOffset);
	}

	bool MeshSerializer::Deserialize(const AssetMetadata_0_1_7* metadata, AssetReference<Asset> destinationAsset) const
	{
		const auto filePath = g_assetManager->GetAssetFilesystemPath(metadata->filepath);

		AssetReference<MeshAsset> meshAsset = destinationAsset.ConvertTo<MeshAsset>();

		if (!std::filesystem::exists(filePath))
		{
			VT_LOG(Error, "File {0} not found!", metadata->filepath);
			meshAsset->SetFlag(AssetFlag::Missing, true);
			return false;
		}

		BinaryStreamReader streamReader{ filePath };

		if (!streamReader.IsStreamValid())
		{
			VT_LOG(Error, "Failed to open file: {0}!", metadata->filepath);
			meshAsset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		SerializedAssetMetadata serializedMetadata = AssetSerializer::ReadMetadata(streamReader);

		if (serializedMetadata.version == 2)
		{
			MeshSerializationData_V2 serializationData{};
			streamReader.Read(serializationData);

			MeshInitializer meshInitializer;

			meshInitializer.SetVertices(
				serializationData.vertexPositions,
				serializationData.vertexMaterialData,
				serializationData.vertexAnimationData
			);

			meshInitializer.SetIndices(serializationData.indices);
			meshInitializer.SetSubMeshes(serializationData.subMeshes);

			meshAsset->Initialize(meshInitializer, serializationData.materials);
		}
		else
		{
			return false;
		}

		return true;
	}
}
