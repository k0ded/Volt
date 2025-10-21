#include "sbpch.h"
#include "Sandbox.h"

#include <RenderCore/Shader/ShaderMap.h>

#include <Volt-Application/UI/UIUtility.h>

#include <Volt-Core/Project/ProjectManager.h>
#include <AssetSystem/AssetTypes.h>

#include <AssetSystem/AssetManager.h>


void Sandbox::CreateModifiedWatch()
{
	m_fileWatcher->AddCallback(efsw::Actions::Modified, [&](const auto newPath, const auto oldPath)
	{
		if (newPath.extension().string() == ".nv-gpudmp" || oldPath.extension().string() == ".nv-gpudmp")
		{
			return;
		}

		if (std::filesystem::is_directory(newPath))
		{
			return;
		}

		std::scoped_lock lock(m_fileWatcherMutex);
		m_fileChangeQueue.emplace_back([newPath, oldPath, this]()
		{
			AssetType assetType = Volt::AssetManager::GetAssetTypeFromPath(newPath);
			if (assetType == AssetTypes::Mesh ||
				assetType == AssetTypes::Prefab ||
				assetType == AssetTypes::Material ||
				assetType == AssetTypes::Texture)
			{
				Volt::AssetManager::Get().ReloadAsset(Volt::AssetManager::GetRelativePath(newPath));
			}
			else if (assetType == AssetTypes::MeshSource)
			{
				/*const auto assets = Volt::AssetManager::GetAllAssetsWithDependency(Volt::AssetManager::Get().GetRelativePath(newPath));
for (const auto& asset : assets)
{
	if (EditorUtils::ReimportSourceMesh(asset))
	{
		UI::Notify(UI::NotificationType::Success, "Re imported mesh!", std::format("Mesh {0} has been reimported!", Volt::AssetManager::GetFilePathFromAssetHandle(asset).string()));
	}
}*/
			}
			else
			{
				if (newPath.extension() == L".hlsl" || newPath.extension() == L".hlsli")
				{
					Volt::ShaderMap::ReloadAllWithReferenceToFile(newPath);
				}
			}
		});
	});
}

void Sandbox::CreateDeleteWatch()
{
	m_fileWatcher->AddCallback(efsw::Actions::Delete, [&](const std::filesystem::path newPath, const std::filesystem::path oldPath)
	{
		if (newPath.extension() == ".tmp" || newPath.extension() == ".TMP")
		{
			return;
		}

		std::scoped_lock lock(m_fileWatcherMutex);
		m_fileChangeQueue.emplace_back([newPath, oldPath]()
		{
			if (!newPath.has_extension())
			{
				Volt::AssetManager::Get().RemoveFullFolderFromRegistry(Volt::AssetManager::GetRelativePath(newPath));
			}
			else
			{
				AssetType assetType = Volt::AssetManager::GetAssetTypeFromPath(Volt::AssetManager::GetRelativePath(newPath));
				if (assetType != AssetTypes::None)
				{
					if (Volt::AssetManager::ExistsInRegistry(Volt::AssetManager::GetRelativePath(newPath)))
					{
						Volt::AssetManager::Get().RemoveAssetFromRegistry(Volt::AssetManager::GetRelativePath(newPath));
					}
				}
			}
		});
	});
}

void Sandbox::CreateAddWatch()
{
	m_fileWatcher->AddCallback(efsw::Actions::Add, [&](const std::filesystem::path newPath, const std::filesystem::path oldPath)
	{
		std::scoped_lock lock(m_fileWatcherMutex);
	});
}

void Sandbox::CreateMovedWatch()
{
	m_fileWatcher->AddCallback(efsw::Actions::Moved, [&](const std::filesystem::path newPath, const std::filesystem::path oldPath)
	{
		std::scoped_lock lock(m_fileWatcherMutex);
		m_fileChangeQueue.emplace_back([newPath, oldPath]()
		{
			if (!newPath.has_extension())
			{
				Volt::AssetManager::Get().MoveFullFolder(oldPath, newPath);
			}
			else
			{
				Volt::AssetManager::Get().MoveAssetInRegistry(oldPath, newPath);
			}
		});
	});
}
