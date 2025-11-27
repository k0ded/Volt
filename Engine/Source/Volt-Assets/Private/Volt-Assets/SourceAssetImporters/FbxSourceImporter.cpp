#include "vtassetspch.h"

#include "Volt-Assets/SourceAssetImporters/FbxSourceImporter.h"
#include "Volt-Assets/SourceAssetImporters/FbxUtility.h"

#include <Volt-Renderer/Mesh/Mesh.h>
#include <Volt-Animation/Assets/Skeleton.h>

#include <Volt-Assets/MaterialAsset.h>
#include <Volt-Assets/MeshAsset.h>

#include <AssetSystem/AssetManager.h>

#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/Packing.h>

#include <fbxsdk.h>
#include <AssetSystem/AssetLocks.h>

VT_DEFINE_LOG_CATEGORY(LogFbxSourceImporter);

using namespace fbxsdk;

namespace Volt
{
	VT_REGISTER_SOURCE_ASSET_IMPORTER(({ ".fbx", ".FBX", ".dxf", ".dae", ".obj", ".3ds"}), FbxSourceImporter);

	using FbxScenePtr = std::unique_ptr<FbxScene, FbxSDKDeleter>;
	using FbxManagerPtr = std::unique_ptr<FbxManager, FbxSDKDeleter>;
	using FbxIOSettingsPtr = std::unique_ptr<FbxIOSettings, FbxSDKDeleter>;
	using FbxImporterPtr = std::unique_ptr<fbxsdk::FbxImporter, FbxSDKDeleter>;

	using VertexDuplicateAccelerationMap = Map<size_t, uint32_t>; // Vertex hash to index

	inline void PostProccessFbxScene(FbxScene* fbxScene, const MeshSourceImportConfig& importConfig, const SourceAssetUserImportData& userData)
	{
		VT_PROFILE_FUNCTION();

		FbxGeometryConverter converter(fbxScene->GetFbxManager());

		if (importConfig.convertScene)
		{
			if (fbxScene->GetGlobalSettings().GetSystemUnit() != FbxSystemUnit::cm)
			{
				constexpr FbxSystemUnit::ConversionOptions sysUnitConversion =
				{
					false,
					true,
					true,
					true,
					true,
					true
				};

				FbxSystemUnit::m.ConvertScene(fbxScene, sysUnitConversion);
				if (fbxScene->GetGlobalSettings().GetSystemUnit() != FbxSystemUnit::cm)
				{
					userData.OnWarning("Failed to convert FBX scene to cm:s. The imported assets may not have the correct scaling.");
				}
			}
		}

		if (importConfig.removeDegeneratePolygons)
		{
			VT_PROFILE_SCOPE("Remove degenerate polygons");

			FbxArray<FbxNode*> removedFromNodes;
			converter.RemoveBadPolygonsFromMeshes(fbxScene, &removedFromNodes);

			const int32_t nodeCount = removedFromNodes.Size();
			for (int i = 0; i < nodeCount; i++)
			{
				const std::string nodeName = GetFbxNodePath(removedFromNodes.GetAt(i));
				userData.OnInfo(std::format("One or more meshes at node {} had degenerate polygons removed!", nodeName));
			}
		}

		if (importConfig.triangulate)
		{
			VT_PROFILE_SCOPE("Triangulate");

			Vector<FbxGeometry*> geomToTriangulate;
			auto findGeomToTriangulate = [&](FbxGeometry* geometry, FbxNode* node)
			{
				VT_ENSURE(geometry);

				if (FbxMesh* mesh = FbxCast<FbxMesh>(geometry))
				{
					if (mesh->IsTriangleMesh())
					{
						return;
					}
				}
				else if (!geometry->Is<FbxNurbsCurve>() && !geometry->Is<FbxNurbsSurface>() && !geometry->Is<FbxPatch>())
				{
					return; // Non convertiable
				}

				geomToTriangulate.emplace_back(geometry);
			};

			VisitNodeAttributesOfType<FbxGeometry>(fbxScene, findGeomToTriangulate);

			if (!geomToTriangulate.empty())
			{
				for (auto* fbxGeom : geomToTriangulate)
				{
					std::string geomName = fbxGeom->GetName();
					if (geomName.empty())
					{
						const FbxNode* node = fbxGeom->GetNode();
						geomName = node ? node->GetName() : "";

						if (geomName.empty())
						{
							geomName = "<anonymous>";
						}
					}

					// If the legacy (faster) triangulation method failed, try the new one
					bool result = converter.Triangulate(fbxGeom, true, true) != nullptr;
					if (!result)
					{
						result = converter.Triangulate(fbxGeom, true, false) != nullptr;
					}

					if (!result)
					{
						userData.OnWarning(std::format("Failed to triangulate {}!", geomName));
					}
				}
			}
		}

		if (importConfig.generateNormalsMode != GenerateNormalsMode::Never)
		{
			VT_PROFILE_SCOPE("Generate normals");

			Vector<FbxMesh*> geomToGenerate;
			const bool overwrite = (importConfig.generateNormalsMode == GenerateNormalsMode::OverwriteSmooth) || (importConfig.generateNormalsMode == GenerateNormalsMode::OverwriteHard);

			auto generator = [&](FbxMesh* mesh, FbxNode* node)
			{
				if (!overwrite)
				{
					const int layerCount = mesh->GetLayerCount();
					for (int i = 0; i < layerCount; i++)
					{
						FbxLayer* fbxLayer = mesh->GetLayer(i);
						if (fbxLayer && fbxLayer->GetNormals())
						{
							return;
						}
					}
				}
				geomToGenerate.emplace_back(mesh);
			};

			VisitNodeAttributesOfType<FbxMesh>(fbxScene, generator);

			if (!geomToGenerate.empty())
			{
				const bool smoothNormals = (importConfig.generateNormalsMode == GenerateNormalsMode::Smooth) || (importConfig.generateNormalsMode == GenerateNormalsMode::OverwriteSmooth);
				const bool clockwise = false;

				for (auto* fbxMesh : geomToGenerate)
				{
					std::string meshName = fbxMesh->GetName();
					if (meshName.empty())
					{
						const FbxNode* node = fbxMesh->GetNode();
						meshName = node ? node->GetName() : "";

						if (meshName.empty())
						{
							meshName = "<anonymous>";
						}
					}

					const bool result = fbxMesh->GenerateNormals(true, smoothNormals, clockwise);
					if (!result)
					{
						userData.OnWarning(std::format("Failed to generate {} normals for {}", (smoothNormals ? "smooth" : "hard"), meshName));
					}
				}
			}
		}

		if (importConfig.generateTangents)
		{
			VT_PROFILE_SCOPE("Generate tangents");

			Vector<FbxMesh*> meshesToGenerate;
			auto generator = [&](FbxMesh* mesh, FbxNode* node)
			{
				const int layerCount = mesh->GetLayerCount();
				for (int i = 0; i < layerCount; i++)
				{
					FbxLayer* fbxLayer = mesh->GetLayer(i);
					if (fbxLayer && fbxLayer->GetTangents() && fbxLayer->GetBinormals())
					{
						return;
					}
				}
				meshesToGenerate.emplace_back(mesh);
			};

			VisitNodeAttributesOfType<FbxMesh>(fbxScene, generator);

			for (auto* fbxMesh : meshesToGenerate)
			{
				std::string meshName = fbxMesh->GetName();
				if (meshName.empty())
				{
					const FbxNode* node = fbxMesh->GetNode();
					meshName = node ? node->GetName() : "";

					if (meshName.empty())
					{
						meshName = "<anonymous>";
					}
				}

				if (!fbxMesh->GenerateTangentsData(0, true, false))
				{
					userData.OnWarning(std::format("Failed to generate tangents/binormals for {}", meshName));
				}
			}
		}

		if (importConfig.convertScene)
		{
			VT_PROFILE_SCOPE("Convert scene");

			fbxsdk::FbxAxisSystem axisSystem(fbxsdk::FbxAxisSystem::DirectX);
			axisSystem.DeepConvertScene(fbxScene);
		}
	}

	inline void SetElementIndices(Vector<FatIndex>& fatIndices, int32_t elementType, const FbxLayerElement* layerElement, const FbxLayerElementArrayTemplate<int32_t>& indexArray, const FbxMesh& mesh)
	{
		VT_PROFILE_FUNCTION();

		const int32_t indexCount = static_cast<int32_t>(fatIndices.size());

		if (layerElement->GetMappingMode() == FbxLayerElement::eByControlPoint)
		{
			if (layerElement->GetReferenceMode() == FbxLayerElement::eDirect)
			{
				for (int32_t i = 0; i < indexCount; i++)
				{
					fatIndices[i].elements[elementType] = fatIndices[i].elements[ElementType::Position];
				}
			}
			else if (layerElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
			{
				for (int32_t i = 0; i < indexCount; i++)
				{
					fatIndices[i].elements[elementType] = indexArray.GetAt(fatIndices[i].elements[ElementType::Position]);
				}
			}
		}
		else if (layerElement->GetMappingMode() == FbxLayerElement::eByPolygonVertex)
		{
			if (layerElement->GetReferenceMode() == FbxLayerElement::eDirect)
			{
				for (int32_t i = 0; i < indexCount; i++)
				{
					fatIndices[i].elements[elementType] = i;
				}
			}
			else if (layerElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
			{
				for (int32_t i = 0; i < indexCount; i++)
				{
					fatIndices[i].elements[elementType] = indexArray.GetAt(i);
				}
			}
		}
		else if (layerElement->GetMappingMode() == FbxLayerElement::eAllSame)
		{
			if (layerElement->GetReferenceMode() == FbxLayerElement::eDirect)
			{
				for (int32_t i = 0; i < indexCount; i++)
				{
					fatIndices[i].elements[elementType] = 0;
				}
			}
			else if (layerElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
			{
				for (int32_t i = 0; i < indexCount; i++)
				{
					fatIndices[i].elements[elementType] = indexArray.GetAt(0);
				}
			}
		}
		else if (layerElement->GetMappingMode() == FbxLayerElement::eByPolygon)
		{
			const int32_t faceCount = mesh.GetPolygonCount();

			if (layerElement->GetReferenceMode() == FbxLayerElement::eDirect)
			{
				int32_t cursor = 0;
				for (int32_t i = 0; i < faceCount; i++)
				{
					for (int32_t j = 0; j < mesh.GetPolygonSize(i); j++)
					{
						fatIndices[cursor++].elements[elementType] = i;
					}
				}
			}
			else if (layerElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
			{
				int32_t cursor = 0;
				for (int32_t i = 0; i < faceCount; i++)
				{
					for (int32_t j = 0; j < mesh.GetPolygonSize(i); j++)
					{
						fatIndices[cursor++].elements[elementType] = indexArray.GetAt(i);
					}
				}
			}
		}
		else
		{
			VT_ASSERT(false);
		}
	}

	inline void TriangulateMesh(const FbxMesh& fbxMesh, const Vector<FatIndex>& input, Vector<FatIndex>& output)
	{
		VT_PROFILE_FUNCTION();

		const int32_t faceCount = fbxMesh.GetPolygonCount();
		int32_t t0 = 0;

		for (int32_t i = 0; i < faceCount; i++)
		{
			const int32_t faceSize = fbxMesh.GetPolygonSize(i);
			int32_t t1 = t0 + 1;
			for (int32_t j = 2; j < faceSize; j++)
			{
				output.emplace_back(input[t0]);
				output.emplace_back(input[t1]);
				output.emplace_back(input[t1 + 1]);
				t1++;
			}

			t0 += faceSize;
		}
	}

	inline Vector<AssetReference<MaterialAsset>> CreateSceneMaterials(FbxScene* fbxScene, MaterialTable& materialTable, const MeshSourceImportConfig& importConfig)
	{
		VT_PROFILE_FUNCTION();

		FbxArray<FbxSurfaceMaterial*> sceneMaterials;
		fbxScene->FillMaterialArray(sceneMaterials);

		Vector<AssetReference<MaterialAsset>> result;

		for (int32_t i = 0; i < sceneMaterials.Size(); i++)
		{
			FbxSurfaceMaterial* fbxMaterial = sceneMaterials[i];
			const char* const name = fbxMaterial->GetName();

			auto it = std::find_if(result.begin(), result.end(), [name](const auto mat)
			{
				ScopedAssetReferenceLock materialLock{ mat };
				return mat->GetAssetName() == name;
			});

			if (it != result.end())
			{
				VT_LOGC(Warning, LogFbxSourceImporter, "Ignoring material with duplicated name '{}'!", name);
				continue;
			}

			AssetReference<MaterialAsset> material = g_assetManager->CreateAsset<MaterialAsset>(name);
			ScopedAssetReferenceLock materialLock{ material };

			result.emplace_back(material);

			materialTable.SetMaterial(material->GetRenderMaterial(), static_cast<uint32_t>(result.size() - 1));
		}

		// Create a dummy material
		if (result.empty())
		{
			AssetReference<MaterialAsset> material = g_assetManager->CreateAsset<MaterialAsset>(importConfig.destinationFilename + "_DummyMat");

			ScopedAssetReferenceLock materialLock{ material };
			result.emplace_back(material);

			materialTable.SetMaterial(material->GetRenderMaterial(), 0);
		}

		return result;
	}

	inline void TranslateNodeToSceneMaterials(const FbxNode* fbxNode, const Vector<AssetReference<MaterialAsset>>& materials, Vector<FbxVertex>& vertices)
	{
		VT_PROFILE_FUNCTION();

		const int32_t nodeMaterialCount = fbxNode->GetMaterialCount();

		Vector<int32_t> materialMap(nodeMaterialCount, -1);

		for (size_t i = 0; i < vertices.size(); i++)
		{
			int32_t materialIndex = 0;
			if (vertices[i].material >= 0 && vertices[i].material < nodeMaterialCount)
			{
				if (materialMap[vertices[i].material] < 0)
				{
					const FbxSurfaceMaterial* const fbxMaterial = fbxNode->GetMaterial(vertices[i].material);
					if (fbxMaterial)
					{
						int32_t j = 0;
						while (j < static_cast<int32_t>(materials.size()))
						{
							ScopedAssetReferenceLock materialLock{ materials[j] };

							// Required because ':' is not allowed in asset names
							std::string fbxMatName = fbxMaterial->GetName();
							fbxMatName.erase(std::remove_if(fbxMatName.begin(), fbxMatName.end(), [](char c) { return c == ':'; }), fbxMatName.end());

							if (fbxMatName == materials[j]->GetAssetName())
							{
								break;
							}

							j++;
						}

						VT_ENSURE(j < materials.size());
						materialMap[vertices[i].material] = j;

					}
					else
					{
						materialMap[vertices[i].material] = 0;
					}
				}

				materialIndex = materialMap[vertices[i].material];
			}

			vertices[i].material = materialIndex;
		}
	}

	inline FbxSkeletonContainer CreateFbxSkeleton(FbxScene* fbxScene)
	{
		VT_PROFILE_FUNCTION();

		Vector<fbxsdk::FbxNode*> skeletonNodes;
		Vector<fbxsdk::FbxNode*> parentNodes;

		auto findParentSkeletonNode = [](FbxNode* node) -> FbxNode*
		{
			FbxNode* parentNode = node->GetParent();

			while (parentNode)
			{
				if (parentNode->GetNodeAttribute() && parentNode->GetNodeAttribute()->GetAttributeType() && parentNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
				{
					return parentNode;
				}

				parentNode = parentNode->GetParent();
			}

			return nullptr;
		};

		auto fetcher = [&](fbxsdk::FbxSkeleton* skeleton, FbxNode* node)
		{
			FbxNode* parentNode = findParentSkeletonNode(node);

			parentNodes.emplace_back(parentNode);
			skeletonNodes.emplace_back(node);
		};

		VisitNodeAttributesOfType<FbxSkeleton>(fbxScene, fetcher);

		Map<FbxNode*, int32_t> nodeToSkeletonIndexMap;

		for (size_t i = 0; i < skeletonNodes.size(); i++)
		{
			nodeToSkeletonIndexMap[skeletonNodes[i]] = static_cast<int32_t>(i);
		}

		FbxSkeletonContainer skeleton{};
		skeleton.joints.resize(skeletonNodes.size());

		for (size_t i = 0; i < skeletonNodes.size(); i++)
		{
			auto& joint = skeleton.joints[i];
			joint.name = GetJointName(skeletonNodes[i]->GetName());
			joint.namespaceName = skeletonNodes[i]->GetName();
			joint.parentIndex = i == 0 ? -1 : nodeToSkeletonIndexMap.at(parentNodes[i]);
		}

		return skeleton;
	}

	void FbxSourceImporter::CreateNonIndexedMesh(const fbxsdk::FbxMesh& fbxMesh, Vector<FbxVertex>& outVertices, const JointVertexLinkMap* jointVertexLinks) const
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(outVertices.empty());

		const int32_t indexCount = fbxMesh.GetPolygonVertexCount();
		const int32_t faceCount = fbxMesh.GetPolygonCount();

		// We only care about layer 0
		const FbxLayer* const layer0 = fbxMesh.GetLayer(0);

		Vector<FatIndex> fatIndices(indexCount);

		// Set position indices
		{
			int32_t cursor = 0;
			for (int32_t i = 0; i < faceCount; ++i)
			{
				for (int32_t j = 0; j < fbxMesh.GetPolygonSize(i); j++)
				{
					fatIndices[cursor].elements[ElementType::Position] = fbxMesh.GetPolygonVertex(i, j);
					fatIndices[cursor].elements[ElementType::AnimationData] = fbxMesh.GetPolygonVertex(i, j);
					cursor++;
				}
			}
		}

		// Set normals
		const FbxLayerElementNormal* const inputNormals = layer0 ? layer0->GetNormals() : nullptr;
		const bool hasNormals = inputNormals != nullptr;
		if (hasNormals)
		{
			SetElementIndices(fatIndices, ElementType::Normal, inputNormals, inputNormals->GetIndexArray(), fbxMesh);
		}
		else
		{
			VT_LOGC(Warning, LogFbxSourceImporter, "Mesh is missing normals, setting to zero vector!");
		}

		// Set tangents
		const FbxLayerElementTangent* const inputTangents = layer0 ? layer0->GetTangents() : nullptr;
		const bool hasTangents = inputTangents != nullptr;
		if (hasTangents)
		{
			SetElementIndices(fatIndices, ElementType::Tangent, inputTangents, inputTangents->GetIndexArray(), fbxMesh);
		}
		else
		{
			VT_LOGC(Warning, LogFbxSourceImporter, "Mesh is missing tangents, setting to zero vector!");
		}

		// Set UVs
		const bool hasTexCoords = layer0 && layer0->GetUVSetCount() > 0;
		if (hasTexCoords)
		{
			const FbxLayerElementUV* const inputTexCoords = layer0->GetUVSets().GetFirst();
			SetElementIndices(fatIndices, ElementType::UV, inputTexCoords, inputTexCoords->GetIndexArray(), fbxMesh);
		}

		// Set materials
		const bool hasMaterials = layer0 && layer0->GetMaterials() != nullptr;
		if (hasMaterials)
		{
			const FbxLayerElementMaterial* const inputMaterials = layer0->GetMaterials();
			SetElementIndices(fatIndices, ElementType::Material, inputMaterials, inputMaterials->GetIndexArray(), fbxMesh);

			// https://help.autodesk.com/view/FBX/2016/ENU/?guid=__cpp_ref_class_fbx_layer_element_material_html
			VT_ENSURE(inputMaterials->GetReferenceMode() == FbxLayerElement::eIndexToDirect);
		}

		Vector<FatIndex> triangles;
		TriangulateMesh(fbxMesh, fatIndices, triangles);
		fatIndices.swap(triangles);

		const size_t triIndexCount = fatIndices.size();

		outVertices.resize(triIndexCount);
		for (int32_t i = 0; i < triIndexCount; i++)
		{
			if (hasMaterials)
			{
				outVertices[i].material = fatIndices[i].elements[ElementType::Material];
			}
			else
			{
				outVertices[i].material = -1;
			}

			outVertices[i].position = FbxUtility::ToVec3(fbxMesh.GetControlPointAt(fatIndices[i].elements[ElementType::Position]));

			if (hasNormals)
			{
				outVertices[i].normal = FbxUtility::ToVec3(inputNormals->GetDirectArray().GetAt(fatIndices[i].elements[ElementType::Normal]));
			}
			else
			{
				outVertices[i].normal = 0.f;
			}

			if (hasTangents)
			{
				outVertices[i].tangent = FbxUtility::ToVec4(inputTangents->GetDirectArray().GetAt(fatIndices[i].elements[ElementType::Tangent]));
			}
			else
			{
				outVertices[i].tangent = 0.f;
			}

			if (hasTexCoords)
			{
				const FbxLayerElementUV* const inputTexCoords = layer0->GetUVSets().GetFirst();
				outVertices[i].texCoords = FbxUtility::ToVec2(inputTexCoords->GetDirectArray().GetAt(fatIndices[i].elements[ElementType::UV]));
				outVertices[i].texCoords.y = 1.f - outVertices[i].texCoords.y;
			}

			if (jointVertexLinks)
			{
				const int32_t animDataIndex = fatIndices[i].elements[ElementType::AnimationData];
				auto [begin, end] = jointVertexLinks->equal_range(static_cast<uint32_t>(animDataIndex));
			
				int32_t index = 0;
				for (auto it = begin; it != end && index < 4; ++it, ++index)
				{
					auto jointLink = it->second;
					outVertices[i].influences[index] = jointLink.jointIndex;
					outVertices[i].weights[index] = jointLink.weight;
				}
			}
		}

		// Set default tex coords
		if (!hasTexCoords)
		{
			for (size_t i = 0; i < triIndexCount; i += 3)
			{
				outVertices[i + 0].texCoords = { 0.f, 0.f };
				outVertices[i + 1].texCoords = { 1.f, 0.f };
				outVertices[i + 2].texCoords = { 1.f, 1.f };
			}
		}

		// Make sure that all the vertices in the same triangle has the same material index
		for (size_t i = 0; i < triIndexCount; i += 3)
		{
			outVertices[i + 2].material = outVertices[i + 1].material = outVertices[i].material;
		}
	}

	AssetReference<Animation> FbxSourceImporter::CreateAnimationFromFbxAnimation(fbxsdk::FbxScene* fbxScene, const FbxSkeletonContainer& fbxSkeleton, const fbxsdk::FbxString& animStackName, const MeshSourceImportConfig& importConfig, const SourceAssetUserImportData& userData) const
	{
		VT_PROFILE_FUNCTION();

		const auto timeMode = fbxScene->GetGlobalSettings().GetTimeMode();
		FbxTakeInfo* fbxTake = fbxScene->GetTakeInfo(animStackName);
		if (!fbxTake)
		{
			userData.OnWarning(std::format("The FBX anim stack {} does not contain any takes", animStackName.Buffer()));
			return {};
		}
		
		const FbxTime start = fbxTake->mLocalTimeSpan.GetStart();
		const FbxTime stop = fbxTake->mLocalTimeSpan.GetStop();

		const auto startFrame = start.GetFrameCount(timeMode);
		const auto endFrame = stop.GetFrameCount(timeMode);

		const FbxLongLong animationLength = endFrame - startFrame + 1;

		AssetReference<Animation> voltAnimation = g_assetManager->CreateAsset<Animation>(importConfig.destinationFilename + "_" + std::string(animStackName.Buffer()));
		ScopedAssetReferenceLock animationLock{ voltAnimation };

		voltAnimation->m_framesPerSecond = FbxUtility::GetFramesPerSecond(timeMode);
		voltAnimation->m_duration = static_cast<float>(animationLength) / static_cast<float>(voltAnimation->m_framesPerSecond);
		voltAnimation->m_frames.resize(animationLength);

		uint32_t localFrameCounter = 0;
		for (FbxLongLong t = startFrame; t <= endFrame; t++)
		{
			FbxTime time;
			time.SetFrame(t, timeMode);

			voltAnimation->m_frames[localFrameCounter].localTRS.resize_uninitialized(fbxSkeleton.joints.size());

			for (size_t j = 0; j < fbxSkeleton.joints.size(); j++)
			{
				FbxNode* jointNode = fbxScene->FindNodeByName(fbxSkeleton.joints[j].namespaceName.c_str());
				const FbxAMatrix localTransform = jointNode->EvaluateLocalTransform(time);

				voltAnimation->m_frames[localFrameCounter].localTRS[j].translation = FbxUtility::ToVec3(localTransform.GetT());
				voltAnimation->m_frames[localFrameCounter].localTRS[j].scale = FbxUtility::ToVec3(localTransform.GetS());
				voltAnimation->m_frames[localFrameCounter].localTRS[j].rotation = FbxUtility::ToQuat(localTransform.GetQ());
			}

			localFrameCounter++;
		}

		return voltAnimation;
	}

	void FbxSourceImporter::CreateSubMeshFromVertexRange(MeshInitializer& meshInitializer, const MeshSourceImportConfig& importConfig, const FbxVertex* vertices, size_t indexCount, const std::string& name) const
	{
		VT_PROFILE_FUNCTION();

		Vector<uint32_t> indices;
		Vector<FbxVertex> uniqueVertices;
		uniqueVertices.reserve(indexCount);
		indices.reserve(indexCount);

		VertexDuplicateAccelerationMap duplicateVerticesMap;
		duplicateVerticesMap.reserve(indexCount);

		uint32_t indexCounter = 0;
		for (size_t i = 0; i < indexCount; i++)
		{
			const size_t hash = vertices[i].GetHash();

			if (!duplicateVerticesMap.contains(hash))
			{
				uniqueVertices.push_back(vertices[i]);
				indices.push_back(indexCounter);
			
				duplicateVerticesMap[hash] = indexCounter;
				indexCounter++;
			}
			else
			{
				indices.push_back(duplicateVerticesMap.at(hash));
			}
		}

		uniqueVertices.shrink_to_fit();
		indices.shrink_to_fit();

		VertexContainer vertexContainer{};
		vertexContainer.Resize(uniqueVertices.size());

		for (size_t i = 0; i < uniqueVertices.size(); i++)
		{
			vertexContainer.positions[i] = uniqueVertices[i].position;
			vertexContainer.materialData[i] = VertexMaterialData::Pack(uniqueVertices[i].normal, uniqueVertices[i].tangent, uniqueVertices[i].texCoords);

			// Setup anim data
			{
				auto& animData = vertexContainer.animationData[i];
				animData.influences = uniqueVertices[i].influences;
				animData.weights = uniqueVertices[i].weights;
			}
		}

		SubMesh subMesh;
		subMesh.vertexStartOffset = meshInitializer.GetNumVertices();
		subMesh.indexStartOffset = meshInitializer.GetNumIndices();
		subMesh.vertexCount = static_cast<uint32_t>(uniqueVertices.size());
		subMesh.indexCount = static_cast<uint32_t>(indices.size());
		subMesh.name = name;
		subMesh.materialIndex = static_cast<uint32_t>(uniqueVertices.front().material);
		subMesh.transform = glm::translate(glm::mat4{ 1.f }, importConfig.translation) * glm::scale(glm::mat4{ 1.f }, importConfig.scale);
		subMesh.GenerateHash();

		meshInitializer.AddSubMesh(subMesh);
		meshInitializer.AddVertices(vertexContainer);
		meshInitializer.AddIndices(indices);
	}

	void FbxSourceImporter::CreateVoltMeshFromFbxMesh(const fbxsdk::FbxMesh& fbxMesh, MeshInitializer& meshInitializer, const Vector<AssetReference<MaterialAsset>>& materials, const MeshSourceImportConfig& importConfig, const JointVertexLinkMap* jointVertexLinks) const
	{
		VT_PROFILE_FUNCTION();

		Vector<FbxVertex> vertices;
		CreateNonIndexedMesh(fbxMesh, vertices, jointVertexLinks);
		TranslateNodeToSceneMaterials(fbxMesh.GetNode(), materials, vertices);

		// Sort vertices by material
		std::sort(vertices.begin(), vertices.end(), [](const auto& lhs, const auto& rhs)
		{
			return lhs.material < rhs.material;
		});

		// Find sub meshes
		struct SubMeshRange
		{
			size_t first;
			size_t last;
		};

		Vector<SubMeshRange> subMeshRanges;

		size_t firstVertex = 0;
		for (size_t i = 1; i < vertices.size(); i++)
		{
			if (vertices[i].material != vertices[firstVertex].material)
			{
				subMeshRanges.emplace_back(firstVertex, i);
				firstVertex = i;
			}
		}
		subMeshRanges.emplace_back(firstVertex, vertices.size());

		// Create sub meshes
		for (const auto& [first, last] : subMeshRanges)
		{
			const size_t indexCount = last - first;
			CreateSubMeshFromVertexRange(meshInitializer, importConfig, &vertices[first], indexCount, fbxMesh.GetName());
		}
	}

	void FbxSourceImporter::CreateVoltSkeletonFromFbxSkeleton(const FbxSkeletonContainer& fbxSkeleton, Skeleton& destinationSkeleton) const
	{
		VT_PROFILE_FUNCTION();

		const size_t jointCount = fbxSkeleton.joints.size();

		destinationSkeleton.m_jointNameToIndex.reserve(jointCount);
		destinationSkeleton.m_inverseBindPose.resize_uninitialized(jointCount);
		destinationSkeleton.m_joints.resize(jointCount);
		destinationSkeleton.m_restPose.resize_uninitialized(jointCount);

		for (size_t i = 0; i < jointCount; i++)
		{
			const auto& fbxJoint = fbxSkeleton.joints[i];

			destinationSkeleton.m_jointNameToIndex[fbxJoint.name] = i;
			destinationSkeleton.m_inverseBindPose[i] = fbxJoint.inverseBindPose;

			auto& restPose = destinationSkeleton.m_restPose[i];
			restPose.translation = fbxJoint.restPose.translation;
			restPose.scale = fbxJoint.restPose.scale;
			restPose.rotation = fbxJoint.restPose.rotation;

			destinationSkeleton.m_joints[i].name = fbxJoint.name;
			destinationSkeleton.m_joints[i].parentIndex = fbxJoint.parentIndex;
		}
	}

	void FbxSourceImporter::FindJointVertexLinksAndSetupSkeleton(const fbxsdk::FbxMesh& fbxMesh, FbxSkeletonContainer& inOutSkeleton, JointVertexLinkMap& outVertexLinks) const
	{
		VT_PROFILE_FUNCTION();

		Vector<FbxAMatrix> bindPoses;
		bindPoses.resize(inOutSkeleton.joints.size());

		FbxNode* fbxNode = fbxMesh.GetNode();

		const FbxVector4 fbxTranslation = fbxNode->GetGeometricTranslation(FbxNode::eSourcePivot);
		const FbxVector4 fbxRotation = fbxNode->GetGeometricRotation(FbxNode::eSourcePivot);
		const FbxVector4 fbxScale = fbxNode->GetGeometricScaling(FbxNode::eSourcePivot);
		const FbxAMatrix rootTransform = FbxAMatrix(fbxTranslation, fbxRotation, fbxScale);

		for (int32_t deformerIndex = 0; deformerIndex < fbxMesh.GetDeformerCount(FbxDeformer::eSkin); deformerIndex++)
		{
			FbxSkin* fbxSkin = FbxCast<FbxSkin>(fbxMesh.GetDeformer(deformerIndex, FbxDeformer::eSkin));
			if (!fbxSkin)
			{
				continue;
			}

			int32_t clusterCount = fbxSkin->GetClusterCount();
			for (int32_t clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++)
			{
				FbxCluster* fbxCluster = fbxSkin->GetCluster(clusterIndex);
				const std::string jointName = GetJointName(fbxCluster->GetLink()->GetName());
				const int32_t jointIndex = inOutSkeleton.GetJointIndexFromName(jointName);

				// Could not find joint in skeleton
				if (jointIndex == -1)
				{
					continue;
				}

				// Get skin binding matrices
				FbxAMatrix transform;
				FbxAMatrix linkTransform;
				FbxAMatrix inverseBindPose;

				fbxCluster->GetTransformMatrix(transform);
				fbxCluster->GetTransformLinkMatrix(linkTransform);

				inverseBindPose = linkTransform.Inverse() * transform * rootTransform;

				bindPoses[jointIndex] = inverseBindPose.Inverse();

				inverseBindPose.Transpose();
				inOutSkeleton.joints[jointIndex].inverseBindPose = FbxUtility::ToMatrix(inverseBindPose);

				// Setup vertex binding info
				int32_t indexCount = fbxCluster->GetControlPointIndicesCount();
				for (int32_t index = 0; index < indexCount; index++)
				{
					int32_t controlPoint = fbxCluster->GetControlPointIndices()[index];
					float weight = static_cast<float>(fbxCluster->GetControlPointWeights()[index]);

					outVertexLinks.insert({ static_cast<uint32_t>(controlPoint), { static_cast<uint32_t>(jointIndex), weight } });
				}
			}
		}

		for (size_t i = 0; i < inOutSkeleton.joints.size(); i++)
		{
			auto& joint = inOutSkeleton.joints[i];

			FbxAMatrix parentTransform;
			if (joint.parentIndex >= 0)
			{
				parentTransform = bindPoses[joint.parentIndex];
			}

			const FbxAMatrix localPose = parentTransform.Inverse() * bindPoses[i];

			joint.restPose.translation = FbxUtility::ToVec3(localPose.GetT());
			joint.restPose.rotation = FbxUtility::ToQuat(localPose.GetQ());
			joint.restPose.scale = FbxUtility::ToVec3(localPose.GetS());
		}
	}

	Vector<AssetReference<Asset>> FbxSourceImporter::ImportAsStaticMesh(fbxsdk::FbxScene* fbxScene, const MeshSourceImportConfig& importConfig, const SourceAssetUserImportData& userData) const
	{
		VT_PROFILE_FUNCTION();

		Vector<FbxMesh*> fbxMeshes;
		auto fetcher = [&](FbxMesh* mesh, FbxNode* node)
		{
			fbxMeshes.emplace_back(mesh);
		};

		VisitNodeAttributesOfType<FbxMesh>(fbxScene, fetcher);

		if (fbxMeshes.empty())
		{
			userData.OnError("The import process failed: File does not contain any meshes!");
			return {};
		}

		MaterialTable materialTable;
		Vector<AssetReference<MaterialAsset>> materials = CreateSceneMaterials(fbxScene, materialTable, importConfig);

		Vector<AssetReference<Asset>> result;

		if (importConfig.combineMeshes)
		{
			MeshInitializer meshInitializer;
			meshInitializer.SetMaterialTable(materialTable);

			AssetReference<MeshAsset> voltMesh = g_assetManager->CreateAsset<MeshAsset>(importConfig.destinationFilename);

			for (auto* fbxMesh : fbxMeshes)
			{
				CreateVoltMeshFromFbxMesh(*fbxMesh, meshInitializer, materials, importConfig, nullptr);
			}

			ScopedAssetReferenceLock meshLock{ voltMesh };
			voltMesh->Initialize(meshInitializer, materials);
			result.emplace_back(voltMesh);
		}
		else
		{
			for (auto* fbxMesh : fbxMeshes)
			{
				AssetReference<MeshAsset> voltMesh = g_assetManager->CreateAsset<MeshAsset>(importConfig.destinationFilename + "_" + fbxMesh->GetName());

				MeshInitializer meshInitializer;
				CreateVoltMeshFromFbxMesh(*fbxMesh, meshInitializer, materials, importConfig, nullptr);

				const uint32_t materialIndex = meshInitializer.GetSubMeshes().at(0).materialIndex;
				meshInitializer.AddMaterial(materialTable.GetMaterial(materialIndex), materialIndex);

				ScopedAssetReferenceLock meshLock{ voltMesh };
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

	Vector<AssetReference<Asset>> FbxSourceImporter::ImportAsSkeletalMesh(fbxsdk::FbxScene* fbxScene, const MeshSourceImportConfig& importConfig, const SourceAssetUserImportData& userData) const
	{
		VT_PROFILE_FUNCTION();

		Vector<FbxMesh*> fbxMeshes;
		auto fetcher = [&](FbxMesh* mesh, FbxNode* node)
		{
			fbxMeshes.emplace_back(mesh);
		};

		VisitNodeAttributesOfType<FbxMesh>(fbxScene, fetcher);

		if (fbxMeshes.empty())
		{
			userData.OnError("The import process failed: File does not contain any meshes!");
			return {};
		}

		FbxSkeletonContainer fbxSkeleton = CreateFbxSkeleton(fbxScene);
		if (fbxSkeleton.joints.empty())
		{
			userData.OnError("The import process failed: File does not contain a skeleton definition!");
			return {};
		}

		JointVertexLinkMap jointVertexLinkMap{};

		// Create mesh
		MaterialTable materialTable;
		Vector<AssetReference<MaterialAsset>> materials = CreateSceneMaterials(fbxScene, materialTable, importConfig);

		Vector<AssetReference<Asset>> result;

		if (importConfig.combineMeshes)
		{
			MeshInitializer meshInitializer;
			meshInitializer.SetMaterialTable(materialTable);

			AssetReference<MeshAsset> voltMesh = g_assetManager->CreateAsset<MeshAsset>(importConfig.destinationFilename);

			for (auto* fbxMesh : fbxMeshes)
			{
				FindJointVertexLinksAndSetupSkeleton(*fbxMesh, fbxSkeleton, jointVertexLinkMap);
				CreateVoltMeshFromFbxMesh(*fbxMesh, meshInitializer, materials, importConfig, &jointVertexLinkMap);
			
				jointVertexLinkMap.clear();
			}

			ScopedAssetReferenceLock meshLock{ voltMesh };
			voltMesh->Initialize(meshInitializer, materials);
			result.emplace_back(voltMesh);
		}
		else
		{
			for (auto* fbxMesh : fbxMeshes)
			{
				AssetReference<MeshAsset> voltMesh = g_assetManager->CreateAsset<MeshAsset>(importConfig.destinationFilename + "_" + fbxMesh->GetName());

				MeshInitializer meshInitializer;
				FindJointVertexLinksAndSetupSkeleton(*fbxMesh, fbxSkeleton, jointVertexLinkMap);
				CreateVoltMeshFromFbxMesh(*fbxMesh, meshInitializer, materials, importConfig, &jointVertexLinkMap);

				const uint32_t materialIndex = meshInitializer.GetSubMeshes().at(0).materialIndex;
				meshInitializer.AddMaterial(materialTable.GetMaterial(materialIndex), materialIndex);

				ScopedAssetReferenceLock meshLock{ voltMesh };
				voltMesh->Initialize(meshInitializer, { materials.at(materialIndex) });

				result.emplace_back(voltMesh);
			
				jointVertexLinkMap.clear();
			}
		}

		// Create skeleton
		AssetReference<Skeleton> voltSkeleton = g_assetManager->CreateAsset<Skeleton>(importConfig.destinationFilename + "_Skeleton");
		ScopedAssetReferenceLock skeletonLock{ voltSkeleton };

		CreateVoltSkeletonFromFbxSkeleton(fbxSkeleton, *voltSkeleton);

		Vector<AssetReference<Asset>> animations;

		if (importConfig.importAnimationIfSkeletalMesh)
		{
			FbxArray<FbxString*> animStackNames;
			fbxScene->FillAnimStackNameArray(animStackNames);

			if (animStackNames.Size() != 0)
			{
				animations = ImportAsAnimation(fbxScene, importConfig, userData);
			}
		}

		result.emplace_back(voltSkeleton);
		
		for (auto& material : materials)
		{
			result.emplace_back(material);
		}

		for (auto& anim : animations)
		{
			result.emplace_back(anim);
		}

		return result;
	}

	Vector<AssetReference<Asset>> FbxSourceImporter::ImportAsAnimation(fbxsdk::FbxScene* fbxScene, const MeshSourceImportConfig& importConfig, const SourceAssetUserImportData& userData) const
	{
		VT_PROFILE_FUNCTION();

		FbxSkeletonContainer fbxSkeleton = CreateFbxSkeleton(fbxScene);
		if (fbxSkeleton.joints.empty())
		{
			userData.OnError("The import process failed: File does not contain a skeleton definition!");
			return {};
		}

		FbxArray<FbxString*> animStackNames;
		fbxScene->FillAnimStackNameArray(animStackNames);

		if (animStackNames.Size() == 0)
		{
			userData.OnError("The import process failed: File does not contain any animation stacks!");
			return {};
		}

		Vector<AssetReference<Asset>> result;

		for (int32_t i = 0; i < animStackNames.Size(); i++)
		{
			auto importedAnim = CreateAnimationFromFbxAnimation(fbxScene, fbxSkeleton, *animStackNames[i], importConfig, userData);
			if (importedAnim)
			{
				result.emplace_back(importedAnim);
			}
		}

		return result;
	}

	Vector<AssetReference<Asset>> FbxSourceImporter::ImportInternal(const std::filesystem::path& filepath, const void* config, const SourceAssetUserImportData& userData) const
	{
		VT_PROFILE_FUNCTION();
		const MeshSourceImportConfig& importConfig = *reinterpret_cast<const MeshSourceImportConfig*>(config);

		FbxManagerPtr fbxManager(FbxManager::Create());
		if (!fbxManager)
		{
			VT_LOGC(Error, LogFbxSourceImporter, "Creating the FBX manager failed!");
			userData.OnError("The import process failed!");

			return {};
		}

		FbxIOSettingsPtr fbxIOSettings(FbxIOSettings::Create(fbxManager.get(), "FBX I/O settings"));
		if (!fbxIOSettings)
		{
			VT_LOGC(Error, LogFbxSourceImporter, "Creating FBX IO settings failed!");
			userData.OnError("The import process failed!");

			return {};
		}

		SetupIOSettings(*fbxIOSettings, importConfig, filepath);
		fbxManager->SetIOSettings(fbxIOSettings.get());

		FbxImporterPtr fbxImporter(fbxsdk::FbxImporter::Create(fbxManager.get(), "FBX importer"));
		if (!fbxImporter)
		{
			VT_LOGC(Error, LogFbxSourceImporter, "Creating FBX importer failed!");
			userData.OnError("The import process failed!");

			return {};
		}

		// Setup progress..

		if (!fbxImporter->Initialize(filepath.string().c_str(), -1, fbxIOSettings.get()))
		{
			if (fbxImporter->GetStatus() == FbxStatus::ePasswordError && !fbxIOSettings->GetBoolProp(IMP_FBX_PASSWORD_ENABLE, false))
			{
				const std::string password = userData.OnPasswordRrquired();

				if (!password.empty())
				{
					fbxIOSettings->SetStringProp(IMP_FBX_PASSWORD, password.c_str());
					fbxIOSettings->SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

					if (!fbxImporter->Initialize(filepath.string().c_str(), -1, fbxIOSettings.get()))
					{
						VT_LOGC(Error, LogFbxSourceImporter, "Initializing imported for password protected file failed!");
						userData.OnError("The import process failed!");

						return {};
					}
				}
				else
				{
					VT_LOGC(Error, LogFbxSourceImporter, "Initializing imported for password protected file failed, password required!");
					userData.OnError("The import process failed!");

					return {};
				}
			}

			const std::string message = "Cannot initialize FBX importer for " + filepath.string() + ": " + fbxImporter->GetStatus().GetErrorString();
			VT_LOGC(Error, LogFbxSourceImporter, message);
			userData.OnError(message);
			return {};
		}

		FbxScenePtr fbxScene(FbxScene::Create(fbxManager.get(), filepath.string().c_str()));
		if (!fbxScene)
		{
			VT_LOGC(Error, LogFbxSourceImporter, "Creating FBX scene failed!");
			userData.OnError("The import process failed!");

			return {};
		}

		if (!fbxImporter->Import(fbxScene.get()))
		{
			VT_LOGC(Error, LogFbxSourceImporter, "Importing FBX scene failed!");
			userData.OnError("The import process failed!");

			return {};
		}

		fbxManager->SetIOSettings(nullptr);
		fbxIOSettings.reset();
		fbxImporter.reset();

		PostProccessFbxScene(fbxScene.get(), importConfig, userData);

		Vector<AssetReference<Asset>> result;

		switch (importConfig.importType)
		{
			case MeshSourceImportType::StaticMesh:
			{
				result = ImportAsStaticMesh(fbxScene.get(), importConfig, userData);
				break;
			}

			case MeshSourceImportType::SkeletalMesh:
			{
				result = ImportAsSkeletalMesh(fbxScene.get(), importConfig, userData);
				break;
			}

			case MeshSourceImportType::Animation:
			{
				result = ImportAsAnimation(fbxScene.get(), importConfig, userData);
				break;
			}
		}

		return result;
	}

	SourceAssetFileInformation FbxSourceImporter::GetSourceFileInformation(const std::filesystem::path& filepath) const
	{
		VT_PROFILE_FUNCTION();

		FbxManagerPtr fbxManager(FbxManager::Create());
		if (!fbxManager)
		{
			return {};
		}

		FbxIOSettingsPtr fbxIOSettings(FbxIOSettings::Create(fbxManager.get(), "FBX I/O settings"));
		if (!fbxIOSettings)
		{
			return {};
		}

		SetupIOSettings(*fbxIOSettings, {}, filepath);
		fbxManager->SetIOSettings(fbxIOSettings.get());

		FbxImporterPtr fbxImporter(fbxsdk::FbxImporter::Create(fbxManager.get(), "FBX importer"));
		if (!fbxImporter)
		{
			return {};
		}

		// Setup progress..

		if (!fbxImporter->Initialize(filepath.string().c_str(), -1, fbxIOSettings.get()))
		{
			return {};
		}

		FbxScenePtr fbxScene(FbxScene::Create(fbxManager.get(), filepath.string().c_str()));
		if (!fbxScene)
		{
			return {};
		}

		if (!fbxImporter->Import(fbxScene.get()))
		{
			return {};
		}

		SourceAssetFileInformation result{};

		const auto headerInfo = fbxImporter->GetFileHeaderInfo();

		result.fileCreator = headerInfo->mCreator;
		result.fileUnits = GetStringFromSystemUnit(fbxScene->GetGlobalSettings().GetSystemUnit());
		result.fileAxisDirection = GetStringFromAxisSystem(fbxScene->GetGlobalSettings().GetAxisSystem());

		std::string versionString = std::to_string(headerInfo->mFileVersion);
		versionString.insert(versionString.begin() + 1, '.');
		versionString.insert(versionString.begin() + 3, '.');
		versionString.insert(versionString.begin() + 5, '.');

		result.fileVersion = versionString;

		// Find all mesh nodes
		{
			Vector<FbxMesh*> fbxMeshes;
			auto fetcher = [&](FbxMesh* mesh, FbxNode* node)
			{
				fbxMeshes.emplace_back(mesh);
			};

			VisitNodeAttributesOfType<FbxMesh>(fbxScene.get(), fetcher);
		
			result.hasMesh = !fbxMeshes.empty();
		}

		// Find all skeleton nodes
		{
			Vector<FbxSkeleton*> fbxSkeletons;
			auto fetcher = [&](fbxsdk::FbxSkeleton* skeleton, FbxNode* node)
			{
				fbxSkeletons.emplace_back(skeleton);
			};

			VisitNodeAttributesOfType<FbxSkeleton>(fbxScene.get(), fetcher);

			result.hasSkeleton = !fbxSkeletons.empty();
		}

		result.hasAnimation = result.hasSkeleton && fbxImporter->GetAnimStackCount() > 0;

		return result;
	}
}
