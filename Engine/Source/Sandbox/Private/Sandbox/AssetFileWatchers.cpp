#include "sbpch.h"
#include "Sandbox.h"

#include "Sandbox/EditorAssetManager.h"

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

		//if (!std::filesystem::exists(newPath))
		//{
		//	return;
		//}
		//
		//if (std::filesystem::is_directory(newPath))
		//{
		//	return;
		//}
		//
		//std::scoped_lock lock(m_fileWatcherMutex);
		//m_fileChangeQueue.emplace_back([newPath, oldPath, this]()
		//{
		//	if (newPath.extension() == L".hlsl" || newPath.extension() == L".hlsli")
		//	{
		//		Volt::ShaderMap::ReloadAllWithReferenceToFile(newPath);
		//	}
		//});
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
				g_editorAssetManager->DeleteDirectory(newPath);
			}
			else
			{
				Volt::AssetHandle assetHandle = g_assetManager->GetAssetHandleFromFilepath(newPath);
				if (g_assetManager->IsValidAssetHandle(assetHandle))
				{
					g_editorAssetManager->DeleteAsset(assetHandle);
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
			// It's a shader file.
			if (newPath.extension() == L".hlsl" || newPath.extension() == L".hlsli")
			{
				Volt::ShaderMap::ReloadAllWithReferenceToFile(newPath);
			}
			else
			{
				if (!newPath.has_extension())
				{
					g_editorAssetManager->MoveDirectoryTo(oldPath, newPath);
				}
				else
				{
					g_editorAssetManager->MoveAssetTo(g_assetManager->GetAssetHandleFromFilepath(oldPath), newPath);
				}
			}
		});
	});
}
