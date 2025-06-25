#include "vtpch.h"

#include "Volt/Asset/SourceAssetImporters/GLTFSourceImporter.h"
#include "Volt/Asset/SourceAssetImporters/ImportConfigs.h"

#include <Volt-Renderer/Mesh/Mesh.h>

#include <Volt-Assets/MaterialAsset.h>
#include <Volt-Assets/MeshAsset.h>

#include <AssetSystem/AssetManager.h>

#include <glm/packing.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE 
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

VT_DEFINE_LOG_CATEGORY(LogGLTFSourceImporter);

namespace Volt
{
	using GLTFNodeIndex = size_t;

	inline Vector<Ref<MaterialAsset>> CreateSceneMaterials(tinygltf::Model& gltfModel, const MeshSourceImportConfig& importConfig)
	{
		Vector<Ref<MaterialAsset>> result;

		for (const auto& mat : gltfModel.materials)
		{
			std::string matName = mat.name;
			if (mat.name.empty())
			{
				matName = importConfig.destinationFilename + "_UnnamnedMaterial";
			}

			result.emplace_back(AssetManager::CreateAsset<MaterialAsset>(importConfig.destinationDirectory, matName));
		}

		if (result.empty())
		{
			result.emplace_back(AssetManager::CreateAsset<MaterialAsset>(importConfig.destinationDirectory, importConfig.destinationFilename + "_DummyMaterial"));
		}

		return result;
	}

	inline void GetMeshNodesFromNode(tinygltf::Model& gltfModel, GLTFNodeIndex nodeIndex, Vector<GLTFNodeIndex>& output)
	{
		const auto& node = gltfModel.nodes[nodeIndex];

		if (node.mesh > -1)
		{
			output.emplace_back(static_cast<GLTFNodeIndex>(nodeIndex));
		}

		for (int32_t childIndex : node.children)
		{
			GetMeshNodesFromNode(gltfModel, static_cast<GLTFNodeIndex>(childIndex), output);
		}
	}

	inline Vector<GLTFNodeIndex> GetSceneMeshNodes(tinygltf::Model& gltfModel)
	{
		Vector<GLTFNodeIndex> result;

		const tinygltf::Scene& gltfScene = gltfModel.scenes[gltfModel.defaultScene];
		for (int32_t nodeIndex : gltfScene.nodes)
		{
			GetMeshNodesFromNode(gltfModel, static_cast<GLTFNodeIndex>(nodeIndex), result);
		}

		return result;
	}

	Vector<Ref<Asset>> GLTFSourceImporter::ImportInternal(const std::filesystem::path& filepath, const void* config, const SourceAssetUserImportData& userData) const
	{
		VT_PROFILE_FUNCTION();
		const MeshSourceImportConfig& importConfig = *reinterpret_cast<const MeshSourceImportConfig*>(config);

		tinygltf::Model gltfInput;
		tinygltf::TinyGLTF gltfContext;

		std::string error, warning;
		bool loaded = false;

		if (filepath.extension().string() == ".glb")
		{
			loaded = gltfContext.LoadBinaryFromFile(&gltfInput, &error, &warning, filepath.string());
		}
		else
		{
			loaded = gltfContext.LoadASCIIFromFile(&gltfInput, &error, &warning, filepath.string());
		}

		if (!loaded && !error.empty())
		{
			const std::string outError = std::format("Unable to load GLTF file {}! Reason: {}", filepath.string().c_str(), error.c_str());
			VT_LOGC(Error, LogGLTFSourceImporter, outError);
			userData.OnError(outError);

			return {};
		}

		if (!warning.empty())
		{
			const std::string outWarning = std::format("Importing GLTF file {} produced warnings: {}", filepath.string().c_str(), warning.c_str());
			VT_LOGC(Warning, LogGLTFSourceImporter, outWarning);
			userData.OnWarning(outWarning);
		}

		Vector<Ref<Asset>> result;

		switch (importConfig.importType)
		{
			case MeshSourceImportType::StaticMesh:
			{
				result = ImportAsStaticMesh(gltfInput, importConfig, userData);
				break;
			}

			case MeshSourceImportType::SkeletalMesh:
			{
				VT_ASSERT(false);
				break;
			}

			case MeshSourceImportType::Animation:
			{
				VT_ASSERT(false);
				break;
			}
		}

		return result;
	}

	SourceAssetFileInformation GLTFSourceImporter::GetSourceFileInformation(const std::filesystem::path& filepath) const
	{
		return SourceAssetFileInformation();
	}

	template<typename T>
	struct GLTFView
	{
		const T* ptr = nullptr;
		size_t count = 0;

		VT_NODISCARD VT_INLINE bool Empty() const
		{
			return count == 0;
		}

		VT_NODISCARD VT_INLINE const T& GetAt(size_t index) const
		{
			static T nullValue = T(0);
			if (index < count)
			{
				return ptr[index];
			}

			return nullValue;
		}
	};

	template<typename T>
	GLTFView<T> GetAttributeViewFromName(const std::string& attributeName, const tinygltf::Primitive& gltfPrimitive, const tinygltf::Model& gltfModel)
	{
		if (!gltfPrimitive.attributes.contains(attributeName))
		{
			return {};
		}

		const tinygltf::Accessor& accessor = gltfModel.accessors[gltfPrimitive.attributes.at(attributeName)];
		const tinygltf::BufferView& view = gltfModel.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = gltfModel.buffers[view.buffer];
		
		GLTFView<T> resultView;
		resultView.ptr = reinterpret_cast<const T*>(&(buffer.data[accessor.byteOffset + view.byteOffset]));
		resultView.count = accessor.count;

		return resultView;
	}

	template<typename T>
	GLTFView<T> GetIndexView(const tinygltf::Primitive& gltfPrimitive, const tinygltf::Model& gltfModel)
	{
		if (gltfPrimitive.indices == -1)
		{
			// Primitive doesn't have any index buffer
			return {};
		}

		const tinygltf::Accessor& accessor = gltfModel.accessors[gltfPrimitive.indices];
		const tinygltf::BufferView& view = gltfModel.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = gltfModel.buffers[view.buffer];

		GLTFView<T> resultView;
		resultView.ptr = reinterpret_cast<const T*>(&(buffer.data[accessor.byteOffset + view.byteOffset]));
		resultView.count = accessor.count;

		return resultView;
	}

	void GetIndices(const tinygltf::Primitive& gltfPrimitive, const tinygltf::Model& gltfModel, Vector<uint32_t>& indices)
	{
		if (gltfPrimitive.indices == -1)
		{
			// Primitive doesn't have any index buffer
			return;
		}

		auto convertIndices = [&]<typename T>()
		{
			GLTFView<T> indicesView = GetIndexView<T>(gltfPrimitive, gltfModel);

			indices.resize_uninitialized(indicesView.count);

			for (size_t i = 0; i < indicesView.count; i++)
			{
				indices[i] = static_cast<uint32_t>(indicesView.GetAt(i));
			}
		};

		const tinygltf::Accessor& accessor = gltfModel.accessors[gltfPrimitive.indices];

		if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_BYTE)
		{
			convertIndices.template operator()<int8_t>();
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
		{
			convertIndices.template operator()<uint8_t>();
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_SHORT)
		{
			convertIndices.template operator()<int16_t>();
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
		{
			convertIndices.template operator()<uint16_t>();
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_INT)
		{
			convertIndices.template operator()<int32_t>();
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
		{
			convertIndices.template operator()<uint32_t>();
		}
		else
		{
			VT_ENSURE(false);
		}
	}

	void GLTFSourceImporter::CreateVoltMeshFromGLTFMesh(const tinygltf::Mesh& gltfMesh, const tinygltf::Node& gltfNode, const tinygltf::Model& gltfModel, Ref<Mesh> destinationMesh, const Vector<Ref<MaterialAsset>>& materials) const
	{
		glm::mat4 nodeTransform = glm::identity<glm::mat4>();

		if (gltfNode.translation.size() >= 3)
		{
			nodeTransform = glm::translate(glm::mat4(1.f), glm::vec3(gltfNode.translation[0], gltfNode.translation[1], gltfNode.translation[2]));
		}

		if (gltfNode.rotation.size() >= 4)
		{
			glm::quat rotation(static_cast<float>(gltfNode.rotation[0]), static_cast<float>(gltfNode.rotation[1]), static_cast<float>(gltfNode.rotation[2]), static_cast<float>(gltfNode.rotation[3]));
			nodeTransform = nodeTransform * glm::mat4_cast(rotation);
		}

		if (gltfNode.scale.size() >= 3)
		{
			glm::vec3 scale(gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]);
			nodeTransform = nodeTransform * glm::scale(glm::mat4(1.f), scale);
		}

		for (const tinygltf::Primitive& gltfPrimitive : gltfMesh.primitives)
		{
			GLTFView<glm::vec3> vertexPositions = GetAttributeViewFromName<glm::vec3>("POSITION", gltfPrimitive, gltfModel);

			Vector<uint32_t> indexVector;
			GetIndices(gltfPrimitive, gltfModel, indexVector);

			if (vertexPositions.Empty() || indexVector.empty())
			{
				// We must have vertex positions and indices to create the mesh.
				continue;
			}

			GLTFView<glm::vec3> vertexNormals = GetAttributeViewFromName<glm::vec3>("NORMAL", gltfPrimitive, gltfModel);
			GLTFView<glm::vec4> vertexTangents = GetAttributeViewFromName<glm::vec4>("TANGENT", gltfPrimitive, gltfModel);
			GLTFView<glm::vec2> vertexTexCoords = GetAttributeViewFromName<glm::vec2>("TEXCOORD_0", gltfPrimitive, gltfModel);
		
			VertexContainer vertexContainer{};
			vertexContainer.Resize(vertexPositions.count);

			for (size_t i = 0; i < vertexPositions.count; i++)
			{
				vertexContainer.positions[i] = vertexPositions.GetAt(i);
				
				// Setup material data
				{
					auto& materialData = vertexContainer.materialData[i];

					const auto octNormal = Utility::OctNormalEncode(vertexNormals.GetAt(i));

					materialData.normal.x = uint8_t(octNormal.x * 255u);
					materialData.normal.y = uint8_t(octNormal.y * 255u);
					materialData.tangent = Utility::EncodeTangent(vertexNormals.GetAt(i), vertexTangents.GetAt(i));
					materialData.tangentW = vertexTangents.GetAt(i).w;
					materialData.texCoords = glm::packHalf2x16({ vertexTexCoords.GetAt(i).x, vertexTexCoords.GetAt(i).y });
				}
			}

			auto& subMesh = destinationMesh->m_subMeshes.emplace_back();
			subMesh.indexCount = static_cast<uint32_t>(indexVector.size());
			subMesh.vertexCount = static_cast<uint32_t>(vertexPositions.count);
			subMesh.indexStartOffset = static_cast<uint32_t>(destinationMesh->m_indices.size());
			subMesh.vertexStartOffset = static_cast<uint32_t>(destinationMesh->m_vertexContainer.Size());
			subMesh.materialIndex = gltfPrimitive.material == -1 ? 0u : static_cast<uint32_t>(gltfPrimitive.material);
			subMesh.name = gltfNode.name;
			subMesh.transform = nodeTransform;
			subMesh.GenerateHash();

			destinationMesh->m_indices.append(indexVector);
			destinationMesh->m_vertexContainer.Append(vertexContainer);
		}
	}

	Vector<Ref<Asset>> GLTFSourceImporter::ImportAsStaticMesh(tinygltf::Model& gltfModel, const MeshSourceImportConfig importConfig, const SourceAssetUserImportData& userData) const
	{
		Vector<GLTFNodeIndex> gltfMeshNodes = GetSceneMeshNodes(gltfModel);
		if (gltfMeshNodes.empty())
		{
			userData.OnError("The import process failed: File does not contain any meshes!");
			return {};
		}

		Vector<Ref<MaterialAsset>> materials = CreateSceneMaterials(gltfModel, importConfig);
		Ref<MeshAsset> voltMesh = AssetManager::CreateAsset<MeshAsset>(importConfig.destinationDirectory, importConfig.destinationFilename);

		for (const auto& nodeIndex : gltfMeshNodes)
		{
			const auto& gltfNode = gltfModel.nodes[nodeIndex];
			CreateVoltMeshFromGLTFMesh(gltfModel.meshes[gltfNode.mesh], gltfNode, gltfModel, voltMesh->m_mesh, materials);
		}

		for (uint32_t i = 0; i < static_cast<uint32_t>(materials.size()); i++)
		{
			voltMesh->m_mesh->m_materialTable.SetMaterial(materials[i]->GetRenderMaterial(), i);
		}

		voltMesh->FinalizeDeserialization();

		Vector<Ref<Asset>> result;
		result.emplace_back(voltMesh);

		for (auto& material : materials)
		{
			result.emplace_back(material);
		}

		return result;
	}
}
