#include "sbpch.h"
#include "Sandbox.h"

#include "Sandbox/EditorAssetManager.h"

#include <RenderCore/Shader/GlobalShaderMap.h>

#include <Volt-Application/UI/UIUtility.h>

#include <CoreModule/Project/ProjectManager.h>
#include <AssetSystem/AssetTypes.h>

#include <AssetSystem/AssetManager.h>


void Sandbox::CreateModifiedWatch()
{
	m_fileWatcher->AddCallback(efsw::Actions::Modified, [&](const auto newPath, const auto oldPath)
	{
		if (newPath.ExtensionView() == L".nv-gpudmp" || oldPath.ExtensionView() == L".nv-gpudmp")
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
	m_fileWatcher->AddCallback(efsw::Actions::Delete, [&](const Filesystem::Path newPath, const Filesystem::Path oldPath)
	{
		if (newPath.ExtensionView() == L".tmp" || newPath.ExtensionView() == L".TMP")
		{
			return;
		}

		std::scoped_lock lock(m_fileWatcherMutex);
		m_fileChangeQueue.emplace_back([newPath, oldPath]()
		{
			if (!newPath.HasExtension())
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
	m_fileWatcher->AddCallback(efsw::Actions::Add, [&](const Filesystem::Path newPath, const Filesystem::Path oldPath)
	{
		std::scoped_lock lock(m_fileWatcherMutex);
	});
}

void Sandbox::CreateMovedWatch()
{
	m_fileWatcher->AddCallback(efsw::Actions::Moved, [&](const Filesystem::Path newPath, const Filesystem::Path oldPath)
	{
		std::scoped_lock lock(m_fileWatcherMutex);
		m_fileChangeQueue.emplace_back([newPath, oldPath]()
		{
			// It's a shader file.
			if (newPath.ExtensionView() == L".hlsl" || newPath.ExtensionView() == L".hlsli")
			{
				Volt::GlobalShaderMap::ReloadAllWithReferenceToFile(newPath);
			}
			else
			{
				if (!newPath.HasExtension())
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
