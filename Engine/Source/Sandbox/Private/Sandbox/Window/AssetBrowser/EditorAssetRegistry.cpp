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
#include <AssetSystem/AssetLocks.h>

#include <CoreUtilities/StringUtility.h>

#define ASSET_BROWSER_POPUP_DATA_FUNCTION_IDENTIFIER(aAssetHandleVarName) [](Volt::AssetHandle aAssetHandleVarName)->Vector<std::pair<std::string, std::string>>



/*
		EditorAssetData(
			ASSET_BROWSER_POPUP_DATA_FUNCTION_IDENTIFIER(aAssetHandle)
			{
				auto asset = Volt::AssetManager::GetAsset<Volt::Animation>(aAssetHandle);
				Vector<std::pair<std::string, std::string>> data =
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
					ScopedAssetReferenceLock meshLock{ meshAsset };

					std::filesystem::path sourceMeshPath = "Could not find the source mesh path";

					Vector<std::pair<std::string, std::string>> data =
					{
						std::make_pair("Submesh Count", std::to_string(meshAsset->GetMesh()->GetSubMeshes().size())),

						std::make_pair("Vertex Count", Utility::ToStringWithThousandSeparator(meshAsset->GetMesh()->GetVertexCount())),
						std::make_pair("Index Count", Utility::ToStringWithThousandSeparator(meshAsset->GetMesh()->GetIndexCount())),
						std::make_pair("Source Mesh Path", sourceMeshPath.string())
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
					ScopedAssetReferenceLock lock{ animationAsset };

					Vector<std::pair<std::string, std::string>> data =
					{
						std::make_pair("Duration", std::to_string(animationAsset->GetDuration()) + " seconds"),
						std::make_pair("Frame Count", Utility::ToStringWithThousandSeparator(animationAsset->GetFrameCount())),
						std::make_pair("Frames Per Second", std::to_string(animationAsset->GetFramesPerSecond())),
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
					ScopedAssetReferenceLock lock{ skeletonAsset };

					Vector<std::pair<std::string, std::string>> data =
					{
						std::make_pair("Joint Count", std::to_string(skeletonAsset->GetJointCount()))
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
					ScopedAssetReferenceLock lock{ textureAsset };

					Vector<std::pair<std::string, std::string>> data =
					{
						std::make_pair("Width", std::to_string(textureAsset->GetWidth())),
						std::make_pair("Height", std::to_string(textureAsset->GetHeight())),
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
					ScopedAssetReferenceLock lock{ sceneAsset };

					const auto& stats = sceneAsset->GetStatistics();
					Vector<std::pair<std::string, std::string>> data =
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
		//		Vector<std::pair<std::string, std::string>> data =
		//		{
		//			std::make_pair("Static Friction", std::to_string(asset->staticFriction)),
		//			std::make_pair("Dynamic Friction", std::to_string(asset->dynamicFriction)),
		//			std::make_pair("Bounciness", std::to_string(asset->bounciness)),
		//		};
		//		return data;
		//	})
	}
};

Vector<std::pair<std::string, std::string>> EditorAssetRegistry::GetAssetBrowserPopupData(AssetType aAssetType, Volt::AssetHandle aAssetHandle)
{
	if (myAssetData.contains(aAssetType))
	{
		if (myAssetData[aAssetType].assetBrowserPopupDataFunction)
		{
			return myAssetData[aAssetType].assetBrowserPopupDataFunction(aAssetHandle);
		}
	}
	return Vector<std::pair<std::string, std::string>>();
}

void EditorAssetRegistry::RegisterAssetBrowserPopupData(AssetType aAssetType, AssetBrowserPopupDataFunction aAssetBrowserPopupDataFunction)
{
	if (myAssetData[aAssetType].assetBrowserPopupDataFunction)
	{
		VT_LOG(Warning, "Asset type {0} already has a popup data function registered, overwriting the function.", aAssetType->GetName());
	}
	myAssetData[aAssetType].assetBrowserPopupDataFunction = aAssetBrowserPopupDataFunction;
}
