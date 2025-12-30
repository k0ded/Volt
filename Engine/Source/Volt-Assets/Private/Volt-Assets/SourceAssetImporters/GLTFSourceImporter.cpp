#include "vtassetspch.h"

#include "Volt-Assets/SourceAssetImporters/GLTFSourceImporter_New.h"
#include "Volt-Assets/SourceAssetImporters/ImportConfigs.h"
#include "Volt-Assets/SourceAssetImporters/TangentGenerator.h"
#include "Volt-Assets/SourceAssetImporters/TextureImportCommon.h"

#include "Volt-Assets/MaterialAsset.h"
#include "Volt-Assets/MeshAsset.h"

#include <Volt-Renderer/Mesh/Mesh.h>

#include <Volt-MaterialGraph/MaterialGraph.h>
#include <Volt-MaterialGraph/Nodes/PBROutputNode.h>
#include <Volt-MaterialGraph/Nodes/ConstantNodes.h>
#include <Volt-MaterialGraph/Nodes/MathNodes.h>
#include <Volt-MaterialGraph/Nodes/Texture/SampleTextureNode.h>

#include <AssetSystem/SourceAssetManager.h>

#include <Mosaic/MosaicGraphBuilder.h>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>

#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/Math/TQS.h>

VT_DEFINE_LOG_CATEGORY(LogGLTFSourceImporter);

namespace Volt
{
	VT_REGISTER_SOURCE_ASSET_IMPORTER(({ ".gltf", ".glb" }), GLTFSourceImporter);

	void ImportGLTFMaterialWithTextures(const fastgltf::Material& gltfMaterial, AssetReference<MaterialAsset> material, const Vector<AssetReference<Asset>>& importedTextures)
	{
		Ref<MaterialGraph> materialGraph = material->GetMaterialGraph();

		Mosaic::MosaicGraphBuilder mosaicBuilder(materialGraph->GetMosaicGraphMutable());

		UUID64 baseColorFactorNode = 0;
		{
			glm::vec4 baseColor = 0.f;
			for (uint32_t i = 0; i < 4; ++i)
			{
				baseColor[i] = gltfMaterial.pbrData.baseColorFactor[i];
			}
			
			baseColorFactorNode = mosaicBuilder.AddNode<MosaicNodes::Color4>();
			mosaicBuilder.SetNodeParameterData(baseColorFactorNode, "RGBA", baseColor);
		}

		auto tryGetTextureAtIndex = [&](size_t index)
		{
			if (index < importedTextures.size())
			{
				return importedTextures.at(index)->GetAssetHandle();
			}

			return Asset::Null();
		};

		UUID64 baseColorTextureNode = 0;
		if (gltfMaterial.pbrData.baseColorTexture.has_value())
		{
			baseColorTextureNode = mosaicBuilder.AddNode<MosaicNodes::SampleTextureNode>();
			MosaicNodes::SampleTextureNode& textureNode = mosaicBuilder.GetNodeAsType<MosaicNodes::SampleTextureNode>(baseColorTextureNode);

			textureNode.SetTextureHandle(tryGetTextureAtIndex(gltfMaterial.pbrData.baseColorTexture.value().textureIndex));
		}

		// Metallic and Roughness
		UUID64 metallicFactorNode = 0;
		UUID64 roughnessFactorNode = 0;
		{
			const float metallicFactor = static_cast<float>(gltfMaterial.pbrData.metallicFactor);
			const float roughnessFactor = static_cast<float>(gltfMaterial.pbrData.roughnessFactor);

			metallicFactorNode = mosaicBuilder.AddNode<MosaicNodes::ConstantFloat>();
			mosaicBuilder.SetNodeParameterData(metallicFactorNode, "Value", metallicFactor);

			roughnessFactorNode = mosaicBuilder.AddNode<MosaicNodes::ConstantFloat>();
			mosaicBuilder.SetNodeParameterData(roughnessFactorNode, "Value", roughnessFactor);
		}

		// Metallic roughness texture
		UUID64 metallicRoughnessTextureNode = 0;
		if (gltfMaterial.pbrData.metallicRoughnessTexture.has_value())
		{
			metallicRoughnessTextureNode = mosaicBuilder.AddNode<MosaicNodes::SampleTextureNode>();
			MosaicNodes::SampleTextureNode& textureNode = mosaicBuilder.GetNodeAsType<MosaicNodes::SampleTextureNode>(metallicRoughnessTextureNode);

			textureNode.SetTextureHandle(tryGetTextureAtIndex(gltfMaterial.pbrData.metallicRoughnessTexture.value().textureIndex));
		}

		// Emissive
		UUID64 emissiveFactorNode = 0;
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
		if (gltfMaterial.emissiveTexture.has_value())
		{
			emissiveTextureNode = mosaicBuilder.AddNode<MosaicNodes::SampleTextureNode>();
			MosaicNodes::SampleTextureNode& textureNode = mosaicBuilder.GetNodeAsType<MosaicNodes::SampleTextureNode>(emissiveTextureNode);

			textureNode.SetTextureHandle(tryGetTextureAtIndex(gltfMaterial.emissiveTexture.value().textureIndex));
		}

		UUID64 normalTextureNode = 0;
		if (gltfMaterial.normalTexture.has_value())
		{
			normalTextureNode = mosaicBuilder.AddNode<MosaicNodes::SampleTextureNode>();
			MosaicNodes::SampleTextureNode& textureNode = mosaicBuilder.GetNodeAsType<MosaicNodes::SampleTextureNode>(normalTextureNode);

			textureNode.SetTextureHandle(tryGetTextureAtIndex(gltfMaterial.normalTexture.value().textureIndex));
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
				mosaicBuilder.LinkNodeParameters(metallicRoughnessTextureNode, multiplyNode, "B", "A");
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

	inline Vector<AssetReference<MaterialAsset>> CreateSceneMaterials(fastgltf::Asset& gltfAsset, MaterialTable& materialTable, const MeshSourceImportConfig& importConfig, const Vector<AssetReference<Asset>>& importedTextures)
	{
		VT_PROFILE_FUNCTION();

		Vector<AssetReference<MaterialAsset>> result;

		for (const auto& gltfMaterial : gltfAsset.materials)
		{
			std::string matName = gltfMaterial.name.c_str();
			if (matName.empty())
			{
				matName = importConfig.destinationFilename + "_UnnamnedMaterial";
			}

			AssetReference<MaterialAsset> material = g_assetManager->CreateAsset<MaterialAsset>(matName);
			ImportGLTFMaterialWithTextures(gltfMaterial, material, importedTextures);

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

	inline Vector<size_t> GetGLTFSceneMeshNodes(fastgltf::Asset& gltfAsset)
	{
		Vector<size_t> result;

		for (size_t i = 0; i < gltfAsset.nodes.size(); ++i)
		{
			if (gltfAsset.nodes[i].meshIndex.has_value())
			{
				result.emplace_back(i);
			}
		}

		return result;
	}

	inline Map<size_t, TQS> EvaluateNodeGlobalTransform(fastgltf::Asset& gltfAsset, const MeshSourceImportConfig& importConfig)
	{
		Map<size_t, TQS> result;

		struct StackEntry
		{
			size_t index;
			int32_t parentIndex;
		};

		Vector<StackEntry> stack;

		for (size_t nodeIndex : gltfAsset.scenes[0].nodeIndices)
		{
			stack.emplace_back(nodeIndex, -1);
		}

		while (!stack.empty())
		{
			StackEntry stackEntry = stack.back();
			stack.pop_back();

			const fastgltf::Node& gltfNode = gltfAsset.nodes[stackEntry.index];
			TQS nodeTransform;

			if (auto* trs = std::get_if<fastgltf::TRS>(&gltfNode.transform))
			{
				nodeTransform.translation = { trs->translation[0], trs->translation[1], trs->translation[2] };
				nodeTransform.rotation = glm::quat(trs->rotation[3], trs->rotation[0], trs->rotation[1], trs->rotation[2]);
				nodeTransform.scale = { trs->scale[0], trs->scale[1], trs->scale[2] };
			}
			else if (auto* mat = std::get_if<fastgltf::math::fmat4x4>(&gltfNode.transform))
			{
				const fastgltf::math::fmat4x4& m = *mat;
				glm::mat4 transformMat = glm::make_mat4(&m[0][0]);
				Math::Decompose(transformMat, nodeTransform.translation, nodeTransform.rotation, nodeTransform.scale);
			}

			// Root node, assign root transform
			if (stackEntry.parentIndex == -1)
			{
				// Let's not forget converting to radians.
				result[stackEntry.index] = TQS::Combine(TQS::Make(importConfig.translation, glm::radians(importConfig.rotation), importConfig.scale), nodeTransform);
			}
			else
			{
				VT_ENSURE(result.contains(stackEntry.parentIndex));
				result[stackEntry.index] = TQS::Combine(result.at(stackEntry.parentIndex), nodeTransform);
			}

			for (size_t nodeIndex : gltfNode.children)
			{
				stack.emplace_back(nodeIndex, static_cast<int32_t>(stackEntry.index));
			}
		}

		return result;
	}

	Vector<AssetReference<Asset>> GLTFSourceImporter::ImportInternal(const std::filesystem::path& filepath, const void* config, const SourceAssetUserImportData& userData) const
	{
		VT_PROFILE_FUNCTION();
		const MeshSourceImportConfig& importConfig = *reinterpret_cast<const MeshSourceImportConfig*>(config);

		constexpr fastgltf::Extensions supportedExtensions = fastgltf::Extensions::None;

		fastgltf::Parser parser(supportedExtensions);

		auto gltfFile = fastgltf::MappedGltfFile::FromPath(filepath);
		if (!gltfFile)
		{
			const std::string outError = std::format("Unable to load GLTF file {}! Reason: {}", filepath.string(), fastgltf::getErrorMessage(gltfFile.error()));
			VT_LOGC(Error, LogGLTFSourceImporter, outError);
			userData.OnError(outError);

			return {};
		}

		constexpr fastgltf::Options gltfOptions =
			fastgltf::Options::DontRequireValidAssetMember |
			fastgltf::Options::AllowDouble |
			fastgltf::Options::LoadExternalBuffers |
			fastgltf::Options::DecomposeNodeMatrices |
			fastgltf::Options::GenerateMeshIndices;

		auto asset = parser.loadGltf(gltfFile.get(), filepath.parent_path(), gltfOptions);
		if (asset.error() != fastgltf::Error::None)
		{
			const std::string outError = std::format("Unable to load GLTF file {}! Reason: {}", filepath.string(), fastgltf::getErrorMessage(asset.error()));
			VT_LOGC(Error, LogGLTFSourceImporter, outError);
			userData.OnError(outError);

			return {};
		}

		Vector<AssetReference<Asset>> importedTextures = ProcessTextures(asset.get(), filepath.parent_path(), importConfig);

		Vector<AssetReference<Asset>> result;
		result.append(importedTextures);

		switch (importConfig.importType)
		{
			case MeshSourceImportType::StaticMesh:
			{
				result = ImportAsStaticMesh(asset.get(), importConfig, userData, importedTextures);
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
		return {};
	}

	Vector<AssetReference<Asset>> GLTFSourceImporter::ProcessTextures(fastgltf::Asset& gltfAsset, const std::filesystem::path& srcDirectory, const MeshSourceImportConfig& config) const
	{
		Vector<JobFuture<Vector<AssetReference<Asset>>>> importedTextures;

		for (const fastgltf::Image& gltfImage : gltfAsset.images)
		{
			if (const auto* filepath = std::get_if<fastgltf::sources::URI>(&gltfImage.data))
			{
				VT_ENSURE(filepath->fileByteOffset == 0);
				VT_ENSURE(filepath->uri.isLocalPath());

				std::filesystem::path sourceFilepath = std::filesystem::absolute(srcDirectory / filepath->uri.path());

				Volt::TextureSourceImportConfig importConfig;
				importConfig.destinationDirectory = config.destinationDirectory;
				importConfig.destinationFilename = sourceFilepath.stem().string();
				importConfig.generateMipMaps = true;
				importConfig.importMipMaps = true;
				importConfig.compressionType = TextureImport::TryGetTextureCompressionTypeFromFilename(sourceFilepath.stem().string());

				importedTextures.emplace_back(SourceAssetManager::ImportSourceAsset(sourceFilepath, importConfig));
			}
		}

		Vector<AssetReference<Asset>> result;

		for (auto& future : importedTextures)
		{
			result.append(future.Get());
		}

		return result;
	}

	Vector<AssetReference<Asset>> GLTFSourceImporter::ImportAsStaticMesh(fastgltf::Asset& gltfAsset, const MeshSourceImportConfig& config, const SourceAssetUserImportData& userData, const Vector<AssetReference<Asset>>& importedTextures) const
	{
		MaterialTable materialTable;
		Vector<AssetReference<MaterialAsset>> materials = CreateSceneMaterials(gltfAsset, materialTable, config, importedTextures);

		Vector<AssetReference<Asset>> result;

		if (config.combineMeshes)
		{
			MeshInitializer meshInitializer;
			meshInitializer.SetMaterialTable(materialTable);

			AssetReference<MeshAsset> voltMesh = g_assetManager->CreateAsset<MeshAsset>(config.destinationFilename);

			Map<size_t, TQS> gltfNodeGlobalTransform = EvaluateNodeGlobalTransform(gltfAsset, config);
			Vector<size_t> gltfMeshNodes = GetGLTFSceneMeshNodes(gltfAsset);

			for (const auto& nodeIndex : gltfMeshNodes)
			{
				CreateVoltMeshFromGLTFMesh(nodeIndex, gltfAsset, gltfNodeGlobalTransform, meshInitializer, config, materials);
			}

			voltMesh->Initialize(meshInitializer, materials);
			result.emplace_back(voltMesh);
		}
		else
		{

		}

		for (auto& material : materials)
		{
			result.emplace_back(material);
		}

		return result;
	}

	void GLTFSourceImporter::CreateVoltMeshFromGLTFMesh(size_t gltfNodeIndex, const fastgltf::Asset& gltfAsset, const Map<size_t, TQS>& nodeGlobalTransform, MeshInitializer& meshInitializer, const MeshSourceImportConfig& importConfig, const Vector<AssetReference<MaterialAsset>>& materials) const
	{
		const fastgltf::Node& gltfNode = gltfAsset.nodes[gltfNodeIndex];
		const fastgltf::Mesh& gltfMesh = gltfAsset.meshes[gltfNode.meshIndex.value()];

		for (const fastgltf::Primitive& primitive : gltfMesh.primitives)
		{
			// Only allow triangle meshes for now.
			if (primitive.type != fastgltf::PrimitiveType::Triangles)
			{
				continue;
			}

			const fastgltf::Attribute* positionAttr = primitive.findAttribute("POSITION");
			
			// If there is no position attribute there really isn't anything we can do.
			if (positionAttr == primitive.attributes.end())
			{
				continue;
			}

			const fastgltf::Attribute* normalAttr = primitive.findAttribute("NORMAL");
			const fastgltf::Attribute* texCoordAttr = primitive.findAttribute("TEXCOORD_0");
			const fastgltf::Attribute* tangentAttr = primitive.findAttribute("TANGENT");

			const bool hasNormals = normalAttr != primitive.attributes.end();
			const bool hasTexCoords = texCoordAttr != primitive.attributes.end();
			const bool hasTangents = tangentAttr != primitive.attributes.end();

			const fastgltf::Accessor& positionAccessor = gltfAsset.accessors[positionAttr->accessorIndex];

			VertexContainer vertexContainer{};
			vertexContainer.Resize(positionAccessor.count);

			Vector<uint32_t> indices;
			{
				VT_ENSURE_MSG(primitive.indicesAccessor.has_value(), "All meshes should have indices, since we have declared that they should be generated if missing.");

				const fastgltf::Accessor& indiceAccessor = gltfAsset.accessors[primitive.indicesAccessor.value()];
				indices.resize_uninitialized(indiceAccessor.count);

				fastgltf::iterateAccessorWithIndex<uint32_t>(
					gltfAsset,
					indiceAccessor,
					[&](uint32_t indice, size_t index) { indices[index] = indice; });

				for (size_t i = 0; i < indices.size(); i += 3)
				{
					std::swap(indices[i + 1], indices[i + 2]);
				}
			}

			// Add positions
			fastgltf::iterateAccessorWithIndex<glm::vec3>(
				gltfAsset,
				positionAccessor,
				[&](glm::vec3 position, size_t index) { position.z = -position.z; vertexContainer.positions[index] = position; });

			Vector<glm::vec3> tempNormals;
			
			if (hasNormals)
			{
				const fastgltf::Accessor& normalAccessor = gltfAsset.accessors[normalAttr->accessorIndex];
				tempNormals.resize_uninitialized(normalAccessor.count);

				fastgltf::iterateAccessorWithIndex<glm::vec3>(
					gltfAsset,
					normalAccessor,
					[&](glm::vec3 normal, size_t index) { tempNormals[index] = normal; });
			}
			else
			{
				tempNormals = Vector<glm::vec3>(positionAccessor.count, glm::vec3{ 0.f, 1.f, 0.f });
			}

			Vector<glm::vec2> tempTexCoords;

			if (hasTexCoords)
			{
				const fastgltf::Accessor& texCoordAccessor = gltfAsset.accessors[texCoordAttr->accessorIndex];
				tempTexCoords.resize_uninitialized(texCoordAccessor.count);

				fastgltf::iterateAccessorWithIndex<glm::vec2>(
					gltfAsset,
					texCoordAccessor,
					[&](glm::vec2 texCoord, size_t index) { tempTexCoords[index] = texCoord; });
			}
			else
			{
				tempTexCoords = Vector<glm::vec2>(positionAccessor.count, glm::vec2(0.f, 0.f));
			}

			Vector<glm::vec4> tempTangents;
			if (hasTangents)
			{
				const fastgltf::Accessor& tangentAccessor = gltfAsset.accessors[tangentAttr->accessorIndex];
				tempTangents.resize_uninitialized(tangentAccessor.count);
			
				fastgltf::iterateAccessorWithIndex<glm::vec4>(
					gltfAsset,
					tangentAccessor,
					[&](glm::vec4 tangent, size_t index) { tempTangents[index] = tangent; });
			}
			else
			{
				if (importConfig.generateTangents &&
					hasNormals &&
					hasTexCoords)
				{
					tempTangents.resize_uninitialized(positionAccessor.count);

					TangentGenerator::GenerationData generationData;
					generationData.vertexPositions = vertexContainer.positions.data();
					generationData.vertexNormals = tempNormals.data();
					generationData.vertexUvs = tempTexCoords.data();
					generationData.indices = indices.data();
					generationData.indexCount = static_cast<uint32_t>(indices.size());
					generationData.outTangents = tempTangents.data();

					TangentGenerator::GenerateTangents(generationData);
				}
				else
				{
					tempTangents = Vector<glm::vec4>(positionAccessor.count, glm::vec4(1.f, 0.f, 0.f, 1.f));
				}
				
			}

			// Pack vertices
			for (size_t i = 0; i < vertexContainer.positions.size(); ++i)
			{
				tempNormals[i].z = -tempNormals[i].z;
				tempTangents[i].z = -tempTangents[i].z;
				tempTangents[i].w = -tempTangents[i].w;

				vertexContainer.materialData[i] = VertexMaterialData::Pack(tempNormals[i], tempTangents[i], tempTexCoords[i]);
			}

			SubMesh subMesh;
			subMesh.indexStartOffset = meshInitializer.GetNumIndices();
			subMesh.vertexStartOffset = meshInitializer.GetNumVertices();
			subMesh.indexCount = static_cast<uint32_t>(indices.size());
			subMesh.vertexCount = static_cast<uint32_t>(vertexContainer.positions.size());
			subMesh.materialIndex = primitive.materialIndex.has_value() ? static_cast<uint32_t>(primitive.materialIndex.value()) : 0;
			subMesh.name = gltfNode.name;

			const TQS& nodeTransform = nodeGlobalTransform.at(gltfNodeIndex);

			subMesh.transform.rotation = nodeTransform.rotation;
			subMesh.transform.position = nodeTransform.translation;
			subMesh.transform.scale = nodeTransform.scale;

			subMesh.GenerateHash();

			meshInitializer.AddSubMesh(subMesh);
			meshInitializer.AddVertices(vertexContainer);
			meshInitializer.AddIndices(indices);
		}
	}
}
