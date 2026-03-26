#include "sbpch.h"

#include "Window/AssetBrowser/EditorAssetRegistry.h"

#include "Sandbox/EditorAssetManager.h"

#include <Volt-Assets/MeshAsset.h>
#include <Volt-Assets/MaterialAsset.h>

#include "Volt-Animation/Assets/Animation.h"
#include "Volt-Animation/Assets/Skeleton.h"

#include "Volt-Renderer/Mesh/Mesh.h"
#include "Volt-Renderer/Texture/Texture2D.h"

#include "Volt-Scene/Scene.h"

#include <AssetSystem/AssetManager.h>

#include <CoreUtilities/String/StringUtility.h>

#define ASSET_BROWSER_POPUP_DATA_FUNCTION_IDENTIFIER(aAssetHandleVarName) [](Volt::AssetHandle aAssetHandleVarName)->Vector<std::pair<String, String>>



/*
		EditorAssetData(
			ASSET_BROWSER_POPUP_DATA_FUNCTION_IDENTIFIER(aAssetHandle)
			{
				auto asset = Volt::AssetManager::GetAsset<Volt::Animation>(aAssetHandle);
				Vector<std::pair<String, String>> data =
				{
					std::make_pair("TEMPLATE", "TEMPLATE")),
				};
				return data;
			})

*/
std::unordered_map<AssetType, EditorAssetData> EditorAssetRegistry::myAssetData =
{
	{
		AssetTypes::Mesh,
		EditorAssetData(
			ASSET_BROWSER_POPUP_DATA_FUNCTION_IDENTIFIER(aAssetHandle)
			{
				AssetReference<Volt::MeshAsset> meshAsset;
				if (g_assetManager->TryGetAsset(aAssetHandle, meshAsset))
				{
					Filesystem::Path sourceMeshPath = "Could not find the source mesh path";

					Vector<std::pair<String, String>> data =
					{
						std::make_pair("Submesh Count", FormatString("{}", meshAsset->GetMesh()->GetSubMeshes().size())),

						std::make_pair("Vertex Count", Utility::ToStringWithThousandSeparator(meshAsset->GetMesh()->GetVertexCount())),
						std::make_pair("Index Count", Utility::ToStringWithThousandSeparator(meshAsset->GetMesh()->GetIndexCount())),
						std::make_pair("Source Mesh Path", sourceMeshPath.ToString())
					};
					return data;
				}
			
				return {};
			})
	},
	{
		AssetTypes::Animation,
		EditorAssetData(
			ASSET_BROWSER_POPUP_DATA_FUNCTION_IDENTIFIER(aAssetHandle)
			{
				AssetReference<Volt::Animation> animationAsset;
				if (g_assetManager->TryGetAsset(aAssetHandle, animationAsset))
				{
					Vector<std::pair<String, String>> data =
					{
						std::make_pair("Duration", FormatString("{} seconds", animationAsset->GetDuration())),
						std::make_pair("Frame Count", Utility::ToStringWithThousandSeparator(animationAsset->GetFrameCount())),
						std::make_pair("Frames Per Second", FormatString("{}", animationAsset->GetFramesPerSecond())),
					};
					return data;
				}

				return {};
			})
	},
	{
		AssetTypes::Skeleton,
		EditorAssetData(
			ASSET_BROWSER_POPUP_DATA_FUNCTION_IDENTIFIER(aAssetHandle)
			{
				AssetReference<Volt::Skeleton> skeletonAsset;
				if (g_assetManager->TryGetAsset(aAssetHandle, skeletonAsset))
				{
					Vector<std::pair<String, String>> data =
					{
						std::make_pair("Joint Count", FormatString("{}", skeletonAsset->GetJointCount()))
					};
					return data;
				}

				return {};
			})
	},
	{
		AssetTypes::Texture,
		EditorAssetData(
			ASSET_BROWSER_POPUP_DATA_FUNCTION_IDENTIFIER(aAssetHandle)
			{
				AssetReference<Volt::Texture2D> textureAsset;
				if (g_assetManager->TryGetAsset(aAssetHandle, textureAsset))
				{
					Vector<std::pair<String, String>> data =
					{
						std::make_pair("Width", FormatString("{}", textureAsset->GetWidth())),
						std::make_pair("Height", FormatString("{}", textureAsset->GetHeight())),
					};
					return data;
				}

				return {};
			})
	},
	{
		AssetTypes::Material,
		EditorAssetData(
			ASSET_BROWSER_POPUP_DATA_FUNCTION_IDENTIFIER(aAssetHandle)
			{
				return {};
			})
	},
	{
		AssetTypes::Scene,
		EditorAssetData(
			ASSET_BROWSER_POPUP_DATA_FUNCTION_IDENTIFIER(aAssetHandle)
			{
				AssetReference<Volt::Scene> sceneAsset;
				if (g_assetManager->TryGetAsset(aAssetHandle, sceneAsset))
				{
					const auto& stats = sceneAsset->GetStatistics();
					Vector<std::pair<String, String>> data =
					{
						std::make_pair("Entity Count", Utility::ToStringWithThousandSeparator(stats.entityCount)),
					};

					return data;
				}

				return {};
			})
	},
	{
		//AssetTypes::PhysicsMaterial,
		//EditorAssetData(
		//	ASSET_BROWSER_POPUP_DATA_FUNCTION_IDENTIFIER(aAssetHandle)
		//	{
		//		auto asset = Volt::AssetManager::GetAsset<Volt::PhysicsMaterial>(aAssetHandle);
		//		Vector<std::pair<String, String>> data =
		//		{
		//			std::make_pair("Static Friction", std::to_string(asset->staticFriction)),
		//			std::make_pair("Dynamic Friction", std::to_string(asset->dynamicFriction)),
		//			std::make_pair("Bounciness", std::to_string(asset->bounciness)),
		//		};
		//		return data;
		//	})
	}
};

Vector<std::pair<String, String>> EditorAssetRegistry::GetAssetBrowserPopupData(AssetType aAssetType, Volt::AssetHandle aAssetHandle)
{
	if (myAssetData.contains(aAssetType))
	{
		if (myAssetData[aAssetType].assetBrowserPopupDataFunction)
		{
			return myAssetData[aAssetType].assetBrowserPopupDataFunction(aAssetHandle);
		}
	}
	return Vector<std::pair<String, String>>();
}

void EditorAssetRegistry::RegisterAssetBrowserPopupData(AssetType aAssetType, AssetBrowserPopupDataFunction aAssetBrowserPopupDataFunction)
{
	if (myAssetData[aAssetType].assetBrowserPopupDataFunction)
	{
		VT_LOG(Warning, "Asset type {0} already has a popup data function registered, overwriting the function.", aAssetType->GetName());
	}
	myAssetData[aAssetType].assetBrowserPopupDataFunction = aAssetBrowserPopupDataFunction;
}
