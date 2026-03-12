#pragma once

#include "Volt-Assets/SourceAssetImporters/ImportConfigs.h"

#include <AssetSystem/SourceAssetImporter.h>
#include <AssetSystem/SourceAssetImporterRegistry.h>

#include <CoreUtilities/Math/TQS.h>

VT_DECLARE_LOG_CATEGORY(LogFbxSourceImporter, LogVerbosity::Trace);

namespace fbxsdk
{
	class FbxMesh;
	class FbxScene;
	class FbxString;
}

namespace Volt
{
	class MaterialAsset;
	class Mesh;
	class Skeleton;
	class Animation;
	class MeshInitializer;

	struct FbxVertex;
	struct FbxSkeletonContainer;
	struct GPUTransform;

	class FbxSourceImporter final : public SourceAssetImporter
	{
	protected:
		Vector<AssetReference<Asset>> ImportInternal(const std::filesystem::path& filepath, const void* config, const SourceAssetUserImportData& userData) const override;
		SourceAssetFileInformation GetSourceFileInformation(const std::filesystem::path& filepath) const override;

	private:
		struct JointLink
		{
			uint32_t jointIndex;
			float weight;
		};

		using JointVertexLinkMap = std::unordered_multimap<uint32_t, JointLink>;

		void CreateVoltMeshFromFbxMesh(const fbxsdk::FbxMesh& fbxMesh, MeshInitializer& meshInitializer, const Vector<AssetReference<MaterialAsset>>& materials, const MeshSourceImportConfig& importConfig, const JointVertexLinkMap* jointVertexLinks) const;
		void CreateVoltSkeletonFromFbxSkeleton(const FbxSkeletonContainer& fbxSkeleton, Skeleton& destinationSkeleton) const;

		void FindJointVertexLinksAndSetupSkeleton(const fbxsdk::FbxMesh& fbxMesh, FbxSkeletonContainer& inOutSkeleton, JointVertexLinkMap& outVertexLinks) const;
		void CreateSubMeshFromVertexRange(MeshInitializer& meshInitializer, const FbxVertex* vertices, size_t indexCount, const std::string& name) const;

		void CreateNonIndexedMesh(const fbxsdk::FbxMesh& fbxMesh, const TQS& nodeTransform, const JointVertexLinkMap* jointVertexLinks, Vector<FbxVertex>& outVertices) const;

		AssetReference<Animation> CreateAnimationFromFbxAnimation(fbxsdk::FbxScene* fbxScene, const FbxSkeletonContainer& fbxSkeleton, const fbxsdk::FbxString& animStackName, const MeshSourceImportConfig& importConfig, const SourceAssetUserImportData& userData) const;

		Vector<AssetReference<Asset>> ImportAsStaticMesh(fbxsdk::FbxScene* fbxScene, const MeshSourceImportConfig& importConfig, const SourceAssetUserImportData& userData) const;
		Vector<AssetReference<Asset>> ImportAsSkeletalMesh(fbxsdk::FbxScene* fbxScene, const MeshSourceImportConfig& importConfig, const SourceAssetUserImportData& userData) const;
		Vector<AssetReference<Asset>> ImportAsAnimation(fbxsdk::FbxScene* fbxScene, const MeshSourceImportConfig& importConfig, const SourceAssetUserImportData& userData) const;
	};
}
