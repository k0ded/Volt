#include "vtassetspch.h"

#include "Volt-Assets/Serializers/MeshSerializer.h"
#include "Volt-Assets/MeshAsset.h"

#include "Volt-Renderer/Mesh/Mesh.h"

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/Serialization/AssetSerializationCommon.h>

namespace Volt
{
	struct MeshSerializationData_V1
	{
		Vector<AssetHandle> materials;
		Vector<Vertex> vertices;
		Vector<uint32_t> indices;

		glm::vec3 boundingSphereCenter;
		float boundingSphereRadius;

		Vector<SubMesh> subMeshes;

		static void Serialize(BinaryStreamWriter& streamWriter, const MeshSerializationData_V1& data)
		{
			streamWriter.WriteRaw(data.materials);
			streamWriter.WriteRaw(data.vertices);
			streamWriter.WriteRaw(data.indices);
			streamWriter.Write(data.boundingSphereCenter);
			streamWriter.Write(data.boundingSphereRadius);
			streamWriter.Write(data.subMeshes);
		}

		static void Deserialize(BinaryStreamReader& streamReader, MeshSerializationData_V1& outData)
		{
			streamReader.ReadRaw(outData.materials);
			streamReader.ReadRaw(outData.vertices);
			streamReader.ReadRaw(outData.indices);
			streamReader.Read(outData.boundingSphereCenter);
			streamReader.Read(outData.boundingSphereRadius);
			streamReader.Read(outData.subMeshes);
		}
	};

	struct MeshSerializationData_V2
	{
		Vector<glm::vec3> vertexPositions;
		Vector<VertexMaterialData> vertexMaterialData;
		Vector<VertexAnimationInfo> vertexAnimationInfo;
		Vector<VertexAnimationData> vertexAnimationData;
		Vector<uint16_t> vertexBoneInfluences;
		Vector<float> vertexBoneWeights;
		Vector<uint32_t> indices;

		Vector<AssetHandle> materials;

		glm::vec3 boundingSphereCenter;
		float boundingSphereRadius;

		Vector<SubMesh> subMeshes;

		static void Serialize(BinaryStreamWriter& streamWriter, const MeshSerializationData_V2& data)
		{
			streamWriter.WriteRaw(data.vertexPositions);
			streamWriter.WriteRaw(data.vertexMaterialData);
			streamWriter.WriteRaw(data.vertexAnimationInfo);
			streamWriter.WriteRaw(data.vertexAnimationData);
			streamWriter.WriteRaw(data.vertexBoneInfluences);
			streamWriter.WriteRaw(data.vertexBoneWeights);
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
			streamReader.ReadRaw(outData.vertexAnimationInfo);
			streamReader.ReadRaw(outData.vertexAnimationData);
			streamReader.ReadRaw(outData.vertexBoneInfluences);
			streamReader.ReadRaw(outData.vertexBoneWeights);
			streamReader.ReadRaw(outData.indices);
			streamReader.ReadRaw(outData.materials);
			streamReader.Read(outData.boundingSphereCenter);
			streamReader.Read(outData.boundingSphereRadius);
			streamReader.Read(outData.subMeshes);
		}
	};

	void MeshSerializer::Serialize(const AssetMetadata& metadata, StackVector<uint8_t, ASSET_METADATA_SIZE>& customData, const Ref<Asset>& asset) const
	{
		Ref<MeshAsset> meshAsset = std::reinterpret_pointer_cast<MeshAsset>(asset);

		BinaryStreamWriter streamWriter{};
		const size_t compressedDataOffset = AssetSerializer::WriteMetadata(metadata, asset->GetVersion(), streamWriter);

		const auto& meshMaterials = meshAsset->m_materials;

		MeshSerializationData_V2 serializationData{}; 
		for (const auto& [index, handle] : meshMaterials)
		{
			serializationData.materials.emplace_back(handle);
		}

		const auto& vertexContainer = meshAsset->m_mesh->GetVertexContainer();

		serializationData.vertexPositions = vertexContainer.positions;
		serializationData.vertexMaterialData = vertexContainer.materialData;
		serializationData.vertexAnimationInfo = vertexContainer.animationInfo;
		serializationData.vertexAnimationData = vertexContainer.animationData;
		serializationData.vertexBoneInfluences = vertexContainer.boneInfluences;
		serializationData.vertexBoneWeights = vertexContainer.boneWeights;
		serializationData.indices = meshAsset->m_mesh->GetIndices();
		serializationData.boundingSphereCenter = meshAsset->m_mesh->GetBoundingSphere().center;
		serializationData.boundingSphereRadius = meshAsset->m_mesh->GetBoundingSphere().radius;
		serializationData.subMeshes = meshAsset->m_mesh->GetSubMeshes();

		streamWriter.Write(serializationData);

		const auto filePath = AssetManager::GetFilesystemPath(metadata.filePath);
		streamWriter.WriteToDisk(filePath, true, compressedDataOffset);
	}

	bool MeshSerializer::Deserialize(const AssetMetadata& metadata, Ref<Asset> destinationAsset) const
	{
		const auto filePath = AssetManager::GetFilesystemPath(metadata.filePath);

		if (!std::filesystem::exists(filePath))
		{
			VT_LOG(Error, "File {0} not found!", metadata.filePath);
			destinationAsset->SetFlag(AssetFlag::Missing, true);
			return false;
		}

		BinaryStreamReader streamReader{ filePath };

		if (!streamReader.IsStreamValid())
		{
			VT_LOG(Error, "Failed to open file: {0}!", metadata.filePath);
			destinationAsset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		Ref<MeshAsset> meshAsset = std::reinterpret_pointer_cast<MeshAsset>(destinationAsset);

		SerializedAssetMetadata serializedMetadata = AssetSerializer::ReadMetadata(streamReader);
		if (serializedMetadata.version == 1)
		{
			MeshSerializationData_V1 serializationData{};
			streamReader.Read(serializationData);

			for (uint32_t i = 0; const auto & mat : serializationData.materials)
			{
				meshAsset->m_materials.emplace(i, mat);
				AssetManager::AddDependencyToAsset(metadata.handle, mat);
				i++;
			}

			meshAsset->m_mesh->InitializeWithVertices(serializationData.vertices);
			meshAsset->m_mesh->m_indices = serializationData.indices;
			meshAsset->m_mesh->m_boundingSphere.center = serializationData.boundingSphereCenter;
			meshAsset->m_mesh->m_boundingSphere.radius = serializationData.boundingSphereRadius;
			meshAsset->m_mesh->m_subMeshes = serializationData.subMeshes;
		}
		else
		{
			MeshSerializationData_V2 serializationData{};
			streamReader.Read(serializationData);

			for (uint32_t i = 0; const auto & mat : serializationData.materials)
			{
				meshAsset->m_materials.emplace(i, mat);
				AssetManager::AddDependencyToAsset(metadata.handle, mat);
				i++;
			}

			meshAsset->m_mesh->m_vertexContainer.positions = serializationData.vertexPositions;
			meshAsset->m_mesh->m_vertexContainer.materialData = serializationData.vertexMaterialData;
			meshAsset->m_mesh->m_vertexContainer.animationInfo = serializationData.vertexAnimationInfo;
			meshAsset->m_mesh->m_vertexContainer.animationData = serializationData.vertexAnimationData;
			meshAsset->m_mesh->m_vertexContainer.boneInfluences = serializationData.vertexBoneInfluences;
			meshAsset->m_mesh->m_vertexContainer.boneWeights = serializationData.vertexBoneWeights;
			meshAsset->m_mesh->m_indices = serializationData.indices;
			meshAsset->m_mesh->m_boundingSphere.center = serializationData.boundingSphereCenter;
			meshAsset->m_mesh->m_boundingSphere.radius = serializationData.boundingSphereRadius;
			meshAsset->m_mesh->m_subMeshes = serializationData.subMeshes;
		}

		for (auto& subMesh : meshAsset->m_mesh->m_subMeshes)
		{
			subMesh.GenerateHash();
		}

		meshAsset->FinalizeDeserialization();
		return true;
	}
}
