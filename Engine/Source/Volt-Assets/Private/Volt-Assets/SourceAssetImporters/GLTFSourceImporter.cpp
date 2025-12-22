#include "vtassetspch.h"

#include "Volt-Assets/SourceAssetImporters/GLTFSourceImporter.h"
#include "Volt-Assets/SourceAssetImporters/ImportConfigs.h"
#include "Volt-Assets/SourceAssetImporters/TangentGenerator.h"

#include <Volt-Renderer/Mesh/Mesh.h>

#include <Volt-Assets/MaterialAsset.h>
#include <Volt-Assets/MeshAsset.h>

#include <Volt-MaterialGraph/MaterialGraph.h>
#include <Volt-MaterialGraph/Nodes/PBROutputNode.h>
#include <Volt-MaterialGraph/Nodes/ConstantNodes.h>
#include <Volt-MaterialGraph/Nodes/MathNodes.h>
#include <Volt-MaterialGraph/Nodes/Texture/SampleTextureNode.h>

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/SourceAssetManager.h>

#include <Mosaic/MosaicGraphBuilder.h>

#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/Packing.h>

#include <glm/packing.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

VT_DEFINE_LOG_CATEGORY(LogGLTFSourceImporter);

namespace Volt
{
	VT_REGISTER_SOURCE_ASSET_IMPORTER(({ ".gltf", ".glb" }), GLTFSourceImporter);

	using GLTFNodeIndex = size_t;

	struct ImportImageUserData
	{
		std::filesystem::path importDirectory;
		std::filesystem::path destinationDirectory;
		Vector<JobFuture<Vector<AssetReference<Asset>>>> importedTextures;
	};

	static bool LoadImageData(tinygltf::Image* image, const int imageIdx, std::string* err, std::string* warn, int reqWidth, int reqHeight, const unsigned char* bytes, int size, void* userData)
	{
		ImportImageUserData& importUserData = *reinterpret_cast<ImportImageUserData*>(userData);
		VT_UNUSED(importUserData);

		std::filesystem::path sourceFilepath = std::filesystem::absolute(importUserData.importDirectory / image->uri);

		Volt::TextureSourceImportConfig importConfig;
		importConfig.destinationDirectory = importUserData.destinationDirectory;
		importConfig.destinationFilename = sourceFilepath.stem().string();
		importConfig.generateMipMaps = true;
		importConfig.importMipMaps = true;

		importUserData.importedTextures.emplace_back(SourceAssetManager::ImportSourceAsset(sourceFilepath, importConfig));

		return true;
	}

	void ImportGLTFMaterialWithTextures(const tinygltf::Material& gltfMaterial, AssetReference<MaterialAsset> material, const Vector<AssetReference<Asset>>& importedTextures)
	{
		Ref<MaterialGraph> materialGraph = material->GetMaterialGraph();

		Mosaic::MosaicGraphBuilder mosaicBuilder(materialGraph->GetMosaicGraphMutable());

		UUID64 baseColorFactorNode = 0;
		if (!gltfMaterial.pbrMetallicRoughness.baseColorFactor.empty())
		{
			const glm::vec4 baseColor =
			{
				gltfMaterial.pbrMetallicRoughness.baseColorFactor[0],
				gltfMaterial.pbrMetallicRoughness.baseColorFactor[1],
				gltfMaterial.pbrMetallicRoughness.baseColorFactor[2],
				gltfMaterial.pbrMetallicRoughness.baseColorFactor[3]
			};

			baseColorFactorNode = mosaicBuilder.AddNode<MosaicNodes::Color4>();
			mosaicBuilder.SetNodeParameterData(baseColorFactorNode, "RGBA", baseColor);
		}

		UUID64 baseColorTextureNode = 0;
		if (gltfMaterial.pbrMetallicRoughness.baseColorTexture.index != -1)
		{
			baseColorTextureNode = mosaicBuilder.AddNode<MosaicNodes::SampleTextureNode>();
			MosaicNodes::SampleTextureNode& textureNode = mosaicBuilder.GetNodeAsType<MosaicNodes::SampleTextureNode>(baseColorTextureNode);
		
			AssetReference<Asset> texture = importedTextures.at(gltfMaterial.pbrMetallicRoughness.baseColorTexture.index);
			textureNode.SetTextureHandle(texture->GetAssetHandle());
		}

		// Metallic and Roughness
		UUID64 metallicFactorNode = 0;
		UUID64 roughnessFactorNode = 0;
		{
			const float metallicFactor = static_cast<float>(gltfMaterial.pbrMetallicRoughness.metallicFactor);
			const float roughnessFactor = static_cast<float>(gltfMaterial.pbrMetallicRoughness.roughnessFactor);

			metallicFactorNode = mosaicBuilder.AddNode<MosaicNodes::ConstantFloat>();
			mosaicBuilder.SetNodeParameterData(metallicFactorNode, "Value", metallicFactor);

			roughnessFactorNode = mosaicBuilder.AddNode<MosaicNodes::ConstantFloat>();
			mosaicBuilder.SetNodeParameterData(roughnessFactorNode, "Value", roughnessFactor);
		}

		// Metallic roughness texture
		UUID64 metallicRoughnessTextureNode = 0;
		if (gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
		{
			metallicRoughnessTextureNode = mosaicBuilder.AddNode<MosaicNodes::SampleTextureNode>();
			MosaicNodes::SampleTextureNode& textureNode = mosaicBuilder.GetNodeAsType<MosaicNodes::SampleTextureNode>(metallicRoughnessTextureNode);

			AssetReference<Asset> texture = importedTextures.at(gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index);
			textureNode.SetTextureHandle(texture->GetAssetHandle());
		}

		// Emissive
		UUID64 emissiveFactorNode = 0;
		if (!gltfMaterial.emissiveFactor.empty())
		{
			const glm::vec3 emissive =
			{
				gltfMaterial.emissiveFactor[0],
				gltfMaterial.emissiveFactor[1],
				gltfMaterial.emissiveFactor[2]
			};

			emissiveFactorNode = mosaicBuilder.AddNode<MosaicNodes::ConstantFloat3>();
			mosaicBuilder.SetNodeParameterData(emissiveFactorNode, "Value", emissive);
		}

		// Emissive texture
		UUID64 emissiveTextureNode = 0;
		if (gltfMaterial.emissiveTexture.index != -1)
		{
			emissiveTextureNode = mosaicBuilder.AddNode<MosaicNodes::SampleTextureNode>();
			MosaicNodes::SampleTextureNode& textureNode = mosaicBuilder.GetNodeAsType<MosaicNodes::SampleTextureNode>(emissiveTextureNode);

			AssetReference<Asset> texture = importedTextures.at(gltfMaterial.emissiveTexture.index);
			textureNode.SetTextureHandle(texture->GetAssetHandle());
		}

		UUID64 normalTextureNode = 0;
		if (gltfMaterial.normalTexture.index != -1)
		{
			normalTextureNode = mosaicBuilder.AddNode<MosaicNodes::SampleTextureNode>();
			MosaicNodes::SampleTextureNode& textureNode = mosaicBuilder.GetNodeAsType<MosaicNodes::SampleTextureNode>(normalTextureNode);

			AssetReference<Asset> texture = importedTextures.at(gltfMaterial.normalTexture.index);
			textureNode.SetTextureHandle(texture->GetAssetHandle());
		}

		UUID64 pbrOutputNode = mosaicBuilder.AddNode<MosaicNodes::PBROutputNode>();

		// Base color
		if (baseColorTextureNode != 0)
		{
			if (baseColorFactorNode != 0)
			{
				UUID64 multiplyNode = mosaicBuilder.AddNode<MosaicNodes::MultiplyNode>();
				mosaicBuilder.LinkNodeParameters(baseColorTextureNode, multiplyNode, "RGBA", "A");
				mosaicBuilder.LinkNodeParameters(baseColorFactorNode, multiplyNode, "RGBA", "B");
				mosaicBuilder.LinkNodeParameters(multiplyNode, pbrOutputNode, "", "Base Color");
			}
			else
			{
				mosaicBuilder.LinkNodeParameters(baseColorTextureNode, pbrOutputNode, "RGBA", "Base Color");
			}
		}
		else
		{
			if (baseColorFactorNode != 0)
			{
				mosaicBuilder.LinkNodeParameters(baseColorFactorNode, pbrOutputNode, "RGBA", "Base Color");
			}
		}

		// Metallic roughness
		if (metallicRoughnessTextureNode != 0)
		{
			// Metallic
			{
				UUID64 multiplyNode = mosaicBuilder.AddNode<MosaicNodes::MultiplyNode>();
				mosaicBuilder.LinkNodeParameters(metallicRoughnessTextureNode, multiplyNode, "R", "A");
				mosaicBuilder.LinkNodeParameters(metallicFactorNode, multiplyNode, "Value", "B");
				mosaicBuilder.LinkNodeParameters(multiplyNode, pbrOutputNode, "", "Metallic");
			}

			// Roughness
			{
				UUID64 multiplyNode = mosaicBuilder.AddNode<MosaicNodes::MultiplyNode>();
				mosaicBuilder.LinkNodeParameters(metallicRoughnessTextureNode, multiplyNode, "G", "A");
				mosaicBuilder.LinkNodeParameters(roughnessFactorNode, multiplyNode, "Value", "B");
				mosaicBuilder.LinkNodeParameters(multiplyNode, pbrOutputNode, "", "Roughness");
			}
			
		}
		else
		{
			mosaicBuilder.LinkNodeParameters(metallicFactorNode, pbrOutputNode, "Value", "Metallic");
			mosaicBuilder.LinkNodeParameters(roughnessFactorNode, pbrOutputNode, "Value", "Roughness");
		}

		// Emissive
		if (emissiveTextureNode != 0)
		{
			if (emissiveFactorNode != 0)
			{
				UUID64 multiplyNode = mosaicBuilder.AddNode<MosaicNodes::MultiplyNode>();
				mosaicBuilder.LinkNodeParameters(emissiveTextureNode, multiplyNode, "RGB", "A");
				mosaicBuilder.LinkNodeParameters(emissiveFactorNode, multiplyNode, "Value", "B");
				mosaicBuilder.LinkNodeParameters(multiplyNode, pbrOutputNode, "", "Emissive");
			}
			else
			{
				mosaicBuilder.LinkNodeParameters(emissiveTextureNode, pbrOutputNode, "RGB", "Emissive");
			}
		}
		else
		{
			if (emissiveFactorNode != 0)
			{
				mosaicBuilder.LinkNodeParameters(emissiveFactorNode, pbrOutputNode, "Value", "Emissive");
			}
		}

		// Normal
		if (normalTextureNode != 0)
		{
			mosaicBuilder.LinkNodeParameters(normalTextureNode, pbrOutputNode, "RGB", "Normal");
		}
	}

	inline Vector<AssetReference<MaterialAsset>> CreateSceneMaterials(tinygltf::Model& gltfModel, MaterialTable& materialTable, const MeshSourceImportConfig& importConfig, const Vector<AssetReference<Asset>>& importedTextures)
	{
		VT_PROFILE_FUNCTION();

		Vector<AssetReference<MaterialAsset>> result;

		for (const auto& mat : gltfModel.materials)
		{
			std::string matName = mat.name;
			if (mat.name.empty())
			{
				matName = importConfig.destinationFilename + "_UnnamnedMaterial";
			}

			AssetReference<MaterialAsset> material = g_assetManager->CreateAsset<MaterialAsset>(matName);
			ImportGLTFMaterialWithTextures(mat, material, importedTextures);

			result.emplace_back(material);
			materialTable.SetMaterial(material->GetRenderMaterial(), static_cast<uint32_t>(result.size() - 1));
		}

		if (result.empty())
		{
			AssetReference<MaterialAsset> material = g_assetManager->CreateAsset<MaterialAsset>(importConfig.destinationFilename + "_DummyMaterial");
			result.emplace_back(material);

			materialTable.SetMaterial(material->GetRenderMaterial(), 0);
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
		VT_PROFILE_FUNCTION();
		Vector<GLTFNodeIndex> result;

		const tinygltf::Scene& gltfScene = gltfModel.scenes[gltfModel.defaultScene];
		for (int32_t nodeIndex : gltfScene.nodes)
		{
			GetMeshNodesFromNode(gltfModel, static_cast<GLTFNodeIndex>(nodeIndex), result);
		}

		return result;
	}

	Vector<AssetReference<Asset>> GLTFSourceImporter::ImportInternal(const std::filesystem::path& filepath, const void* config, const SourceAssetUserImportData& userData) const
	{
		VT_PROFILE_FUNCTION();
		const MeshSourceImportConfig& importConfig = *reinterpret_cast<const MeshSourceImportConfig*>(config);

		tinygltf::Model gltfInput;
		tinygltf::TinyGLTF gltfContext;

		ImportImageUserData importImageUserData;
		importImageUserData.importDirectory = filepath.parent_path();
		importImageUserData.destinationDirectory = importConfig.destinationDirectory;

		gltfContext.SetImageLoader(&LoadImageData, &importImageUserData);

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

		Vector<AssetReference<Asset>> importedTextures;
		for (auto& future : importImageUserData.importedTextures)
		{
			importedTextures.append(future.Get());
		}

		Vector<AssetReference<Asset>> result;

		switch (importConfig.importType)
		{
			case MeshSourceImportType::StaticMesh:
			{
				result = ImportAsStaticMesh(gltfInput, importConfig, userData, importedTextures);
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
		const uint8_t* ptr = nullptr;
		size_t count = 0;
		size_t stride = 0;

		VT_NODISCARD VT_INLINE bool Empty() const
		{
			return count == 0;
		}

		VT_NODISCARD VT_INLINE const T& GetAt(size_t index) const
		{
			static T nullValue = T(0);
			if (index < count)
			{
				return *reinterpret_cast<const T*>(ptr + index * stride);
			}

			return nullValue;
		}
		
		VT_NODISCARD VT_INLINE const T* GetData() const
		{
			return reinterpret_cast<const T*>(ptr);
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
		resultView.ptr = reinterpret_cast<const uint8_t*>(&(buffer.data[accessor.byteOffset + view.byteOffset]));
		resultView.count = accessor.count;
		resultView.stride = accessor.ByteStride(view) ? accessor.ByteStride(view) : sizeof(T);

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
		resultView.ptr = reinterpret_cast<const uint8_t*>(&(buffer.data[accessor.byteOffset + view.byteOffset]));
		resultView.count = accessor.count;
		resultView.stride = accessor.ByteStride(view) ? accessor.ByteStride(view) : sizeof(T);

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
			convertIndices.template operator() < int8_t > ();
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
		{
			convertIndices.template operator() < uint8_t > ();
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_SHORT)
		{
			convertIndices.template operator() < int16_t > ();
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
		{
			convertIndices.template operator() < uint16_t > ();
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_INT)
		{
			convertIndices.template operator() < int32_t > ();
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
		{
			convertIndices.template operator() < uint32_t > ();
		}
		else
		{
			VT_ENSURE(false);
		}
	}

	void GLTFSourceImporter::CreateVoltMeshFromGLTFMesh(const tinygltf::Mesh& gltfMesh, const tinygltf::Node& gltfNode, const tinygltf::Model& gltfModel, MeshInitializer& meshInitializer, const Vector<AssetReference<MaterialAsset>>& materials) const
	{
		VT_PROFILE_FUNCTION();

		glm::quat nodeRotation = glm::identity<glm::quat>();
		glm::vec3 nodePosition = 0.f;
		glm::vec3 nodeScale = 1.f;

		if (gltfNode.translation.size() >= 3)
		{
			nodePosition = glm::vec3(gltfNode.translation[0], gltfNode.translation[1], gltfNode.translation[2]);
		}

		if (gltfNode.rotation.size() >= 4)
		{
			nodeRotation.x = static_cast<float>(gltfNode.rotation[0]);
			nodeRotation.y = static_cast<float>(gltfNode.rotation[1]);
			nodeRotation.z = static_cast<float>(gltfNode.rotation[2]);
			nodeRotation.w = static_cast<float>(gltfNode.rotation[3]);
		}

		if (gltfNode.scale.size() >= 3)
		{
			nodeScale = glm::vec3(gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]);
		}

		if (gltfNode.matrix.size() == 16)
		{
			Math::Decompose(glm::make_mat4(gltfNode.matrix.data()), nodePosition, nodeRotation, nodeScale);
		}

		for (const tinygltf::Primitive& gltfPrimitive : gltfMesh.primitives)
		{
			// Only allow triangle meshes for now.
			if (gltfPrimitive.mode != TINYGLTF_MODE_TRIANGLES)
			{
				continue;
			}

			GLTFView<glm::vec3> vertexPositions = GetAttributeViewFromName<glm::vec3>("POSITION", gltfPrimitive, gltfModel);

			Vector<uint32_t> indices;
			GetIndices(gltfPrimitive, gltfModel, indices);

			// It's a non indexed mesh, generate a linear index buffer.
			if (indices.empty())
			{
				indices.resize_uninitialized(vertexPositions.count);
				std::iota(indices.begin(), indices.end(), 0);
			}

			GLTFView<glm::vec3> vertexNormals = GetAttributeViewFromName<glm::vec3>("NORMAL", gltfPrimitive, gltfModel);
			GLTFView<glm::vec4> vertexTangents = GetAttributeViewFromName<glm::vec4>("TANGENT", gltfPrimitive, gltfModel);
			GLTFView<glm::vec2> vertexTexCoords = GetAttributeViewFromName<glm::vec2>("TEXCOORD_0", gltfPrimitive, gltfModel);

			// There are no tangents, generate them
			Vector<glm::vec4> generatedTangents;
			if (vertexTangents.Empty())
			{
				// Normals and tex coords are required for generation.
				if (!vertexNormals.Empty() && !vertexTexCoords.Empty())
				{
					generatedTangents.resize_uninitialized(vertexPositions.count);

					TangentGenerator::GenerationData generationData;
					generationData.vertexPositions = vertexPositions.GetData();
					generationData.vertexNormals = vertexNormals.GetData();
					generationData.vertexUvs = vertexTexCoords.GetData();
					generationData.indices = indices.data();
					generationData.indexCount = static_cast<uint32_t>(indices.size());
					generationData.outTangents = generatedTangents.data();

					TangentGenerator::GenerateTangents(generationData);
				}
			}

			VertexContainer vertexContainer{};
			vertexContainer.Resize(vertexPositions.count);

			for (size_t i = 0; i < vertexPositions.count; i++)
			{
				vertexContainer.positions[i] = vertexPositions.GetAt(i);

				const glm::vec3 normal = vertexNormals.Empty() ? glm::vec3(0.f, 1.f, 0.f) : vertexNormals.GetAt(i);
				const glm::vec2 uv = vertexTexCoords.Empty() ? 0.f : vertexTexCoords.GetAt(i);

				glm::vec4 tangent = glm::vec4(1.f, 0.f, 0.f, 1.f);
				if (!vertexTangents.Empty())
				{
					tangent = vertexTangents.GetAt(i);
				}
				else if (!generatedTangents.empty())
				{
					tangent = generatedTangents[i];
				}

				vertexContainer.materialData[i] = VertexMaterialData::Pack(normal, tangent, uv);
			}

			SubMesh subMesh;
			subMesh.indexStartOffset = meshInitializer.GetNumIndices();
			subMesh.vertexStartOffset = meshInitializer.GetNumVertices();
			subMesh.indexCount = static_cast<uint32_t>(indices.size());
			subMesh.vertexCount = static_cast<uint32_t>(vertexPositions.count);
			subMesh.materialIndex = gltfPrimitive.material == -1 ? 0u : static_cast<uint32_t>(gltfPrimitive.material);
			subMesh.name = gltfNode.name;
			subMesh.transform.position = nodePosition;
			subMesh.transform.rotation = nodeRotation;
			subMesh.transform.scale = nodeScale;
			subMesh.GenerateHash();

			meshInitializer.AddSubMesh(subMesh);
			meshInitializer.AddVertices(vertexContainer);
			meshInitializer.AddIndices(indices);
		}
	}

	Vector<AssetReference<Asset>> GLTFSourceImporter::ImportAsStaticMesh(tinygltf::Model& gltfModel, const MeshSourceImportConfig importConfig, const SourceAssetUserImportData& userData, const Vector<AssetReference<Asset>>& importedTextures) const
	{
		VT_PROFILE_FUNCTION();

		Vector<GLTFNodeIndex> gltfMeshNodes = GetSceneMeshNodes(gltfModel);
		if (gltfMeshNodes.empty())
		{
			userData.OnError("The import process failed: File does not contain any meshes!");
			return {};
		}

		MaterialTable materialTable;
		Vector<AssetReference<MaterialAsset>> materials = CreateSceneMaterials(gltfModel, materialTable, importConfig, importedTextures);

		Vector<AssetReference<Asset>> result;
		if (importConfig.combineMeshes)
		{
			MeshInitializer meshInitializer;
			meshInitializer.SetMaterialTable(materialTable);

			AssetReference<MeshAsset> voltMesh = g_assetManager->CreateAsset<MeshAsset>(importConfig.destinationFilename);

			for (const auto& nodeIndex : gltfMeshNodes)
			{
				const auto& gltfNode = gltfModel.nodes[nodeIndex];
				CreateVoltMeshFromGLTFMesh(gltfModel.meshes[gltfNode.mesh], gltfNode, gltfModel, meshInitializer, materials);
			}

			voltMesh->Initialize(meshInitializer, materials);
			result.emplace_back(voltMesh);
		}
		else
		{
			for (const auto& nodeIndex : gltfMeshNodes)
			{
				const auto& gltfNode = gltfModel.nodes[nodeIndex];

				AssetReference<MeshAsset> voltMesh = g_assetManager->CreateAsset<MeshAsset>(importConfig.destinationFilename + "_" + gltfNode.name);

				MeshInitializer meshInitializer;

				CreateVoltMeshFromGLTFMesh(gltfModel.meshes[gltfNode.mesh], gltfNode, gltfModel, meshInitializer, materials);

				const uint32_t materialIndex = meshInitializer.GetSubMeshes().at(0).materialIndex;
				meshInitializer.AddMaterial(materialTable.GetMaterial(materialIndex), materialIndex);

				voltMesh->Initialize(meshInitializer, { materials.at(materialIndex) });

				result.emplace_back(voltMesh);
			}
		}

		for (auto& material : materials)
		{
			result.emplace_back(material);
		}

		return result;
	}
}
