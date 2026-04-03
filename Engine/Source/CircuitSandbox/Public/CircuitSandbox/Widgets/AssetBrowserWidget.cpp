#include "csbpch.h"
#include "AssetBrowserWidget.h"

#include <Circuit/Widgets/BorderWidget.h>
#include <Circuit/Widgets/TextWidget.h>
#include <Circuit/Widgets/Layout/LayoutWidget.h>
#include <Circuit/Widgets/ListViewWidget.h>

#include <CoreModule/Project/ProjectManager.h>
#include <CoreModule/Algorithms.h>

#include <AssetSystem/AssetManager.h>


#include <FileSystemModule/Iterators/RecursiveDirectoryIterator.h>
#include <FileSystemModule/Iterators/DirectoryIterator.h>

#include <Circuit/Widgets/ButtonWidget.h>

#include <CoreUtilities/Filesystem/Path.h>
#include <CoreUtilities/String/StringUtility.h>
#include <CoreUtilities/Profiling/Profiling.h>

AssetBrowserWidget::AssetBrowserWidget()
{
	m_reloading = false;
}

AssetBrowserWidget::~AssetBrowserWidget()
{}

void AssetBrowserWidget::Build(const Arguments& args)
{
	Ref<Circuit::TextWidget> nameWidget = CreateWidget(Circuit::TextWidget)
		.Text("Asset Browser")
		.Size(20.f)
		.Color(CircuitColor(0xffffffff));

	Ref<Circuit::TextWidget> pathTextWidget = CreateWidget(Circuit::TextWidget)
		.Text_Lambda([this]() -> String
	{
		if (m_reloading.load())
		{
			const String reloadingString = FormatString("RELOADING... ({}/{})", m_reloadProgress.load(std::memory_order::relaxed), m_reloadTotalWork.load(std::memory_order::relaxed));
			return reloadingString;
		}

		if (!m_directories.contains(m_currentDirectoryID))
		{
			return "INVALID DIRECTORY OPEN";
		}

		const AssetBrowserDirectory& dir = m_directories[m_currentDirectoryID];
		String result = dir.Path.ToString();
		Filesystem::Path basePath = Volt::ProjectManager::GetAssetsDirectory().ParentPath();
		Utility::RemoveFromStartInline(result, basePath.ToString());
		return result;
	})
		.Size(20.f)
		.Color(CircuitColor(0xffffffff));
	auto pathBorder = CreateWidget(Circuit::BorderWidget)
		.Content(pathTextWidget)
		.Padding(glm::vec4(10, 0, 0, 10));

	Ref<Circuit::TextWidget> upTextWidget = CreateWidget(Circuit::TextWidget)
		.Text("^")
		.Size(30.f)
		.Color(CircuitColor(0xffffffff));

	Ref<Circuit::ButtonWidget> upButtonWidget = CreateWidget(Circuit::ButtonWidget)
		.Content(upTextWidget)
		.OnReleased_Raw(this, &AssetBrowserWidget::OnClickedUp);

	Ref<Circuit::TextWidget> reloadTextWidget = CreateWidget(Circuit::TextWidget)
		.Text("Reload")
		.Size(20.f)
		.Color(CircuitColor(0xffffffff));

	Ref<Circuit::ButtonWidget> reloadButtonWidget = CreateWidget(Circuit::ButtonWidget)
		.Content(reloadTextWidget)
		.OnReleased_Raw(this, &AssetBrowserWidget::OnClickedReload);

	Ref<Circuit::LayoutWidget> topBar = CreateWidget(Circuit::LayoutWidget).Orientation(Circuit::LayoutOrientation::Horizontal);
	topBar->AddFlexibleSlice(nameWidget);
	topBar->AddFlexibleSlice(upButtonWidget);
	topBar->AddFlexibleSlice(pathBorder);
	topBar->AddSpring();
	topBar->AddFlexibleSlice(reloadButtonWidget);

	Ref<Circuit::LayoutWidget> layout = CreateWidget(Circuit::LayoutWidget).Orientation(Circuit::LayoutOrientation::Vertical);
	layout->AddFixedSlice(topBar, 30.f);

	m_assetsListWidget = CreateWidget(Circuit::ListViewWidget<AssetBrowserItemProxy>)
		.ItemsSource(m_currentDirectoryItems)
		.OnGenerateRow_Raw(this, &AssetBrowserWidget::GenerateRow)
		.OnRowDoubleClicked_Raw(this, &AssetBrowserWidget::OnRowDoubleClicked);

	layout->AddFlexibleSlice(m_assetsListWidget);

	auto border = CreateWidget(Circuit::BorderWidget)
		.BackgroundColor(CircuitColor(30, 30, 30))
		.Padding({ 10.f, 10.f, 10.f, 10.f })
		.Content(layout);

	AddChildWidget(border);


	RediscoverFiles();
}

void AssetBrowserWidget::RediscoverFiles()
{
	if (m_reloading.load())
	{
		return;
	}

	{
		bool expectedReloading = false;
		m_reloading.compare_exchange_strong(expectedReloading, true);
	}

	m_currentDirectoryID = 0;
	m_currentDirectoryItems = nullptr;
	m_assetsListWidget->SetItemsSource(nullptr);

	m_reloadProgress.store(0);
	m_reloadTotalWork.store(0);

	auto reloadFunc = [this]()
	{
		m_directories.clear();

		const Filesystem::Path baseDirectoryPath = Volt::ProjectManager::GetAssetsDirectory();

		Vector<Filesystem::Path> directories;
		directories.push_back(baseDirectoryPath);

		{
			VT_PROFILE_SCOPE("FindDirectoriesRecursive");
			int32_t totalWork = 0;
			for (const auto& entry : Filesystem::RecursiveDirectoryIterator(baseDirectoryPath))
			{
				//each entry will be handled once
				totalWork++;
				if (!entry.isDirectory)
				{
					continue;
				}
				//each directory will be handled twice
				totalWork++;
				directories.push_back(entry.path);
			}
			m_reloadTotalWork.store(totalWork);
		}

		{
			VT_PROFILE_SCOPE("ProcessDirectories");
			for (const Filesystem::Path& directoryPath : directories)
			{
				//todo_fabian: should probably use a hash of the path for the ID
				AssetBrowserItemID newDirectoryID{};
				VT_ENSURE(!m_directories.contains(newDirectoryID));
				m_directoryPathToID.insert({ directoryPath, newDirectoryID });
			}

			m_directories.reserve(directories.size());
			for (const Filesystem::Path& directoryPath : directories)
			{
				Volt::JobSystem::YieldFromJob();
				VT_PROFILE_SCOPE(FormatString("ProcessDirectory ({})", directoryPath.ToString()).c_str());
				m_reloadProgress.fetch_add(1);

				AssetBrowserDirectory dir{};
				dir.Path = directoryPath;
				dir.ID = m_directoryPathToID[directoryPath];
				for (const auto& entry : Filesystem::DirectoryIterator(directoryPath))
				{
					m_reloadProgress.fetch_add(1);
					AssetBrowserItemProxy itemProxy;
					itemProxy.ParentDirectoryID = dir.ID;

					if (entry.isDirectory)
					{
						VT_ENSURE(m_directoryPathToID.contains(entry.path));
						itemProxy.ID = m_directoryPathToID[entry.path];

						itemProxy.Type = AssetBrowserItemType::Directory;
					}
					else
					{
						itemProxy.ID = AssetBrowserItemID();
						itemProxy.Type = AssetBrowserItemType::Asset;

						AssetBrowserAssetItem newAssetItem;
						newAssetItem.ID = itemProxy.ID;

						newAssetItem.Path = entry.path;

						dir.Assets.insert({ newAssetItem.ID, std::move(newAssetItem) });
					}

					dir.Children.push_back(std::move(itemProxy));
				}

				m_directories.insert({ dir.ID, std::move(dir) });
			}
		}

		{
			VT_PROFILE_SCOPE("Find Asset handles");

			Map<Filesystem::Path, Volt::AssetHandle> pathToHandle;
			pathToHandle.reserve(g_assetManager->GetNumAssetsInRegistry());

			Volt::AssetRegistryIteratorFilter filter;
			filter.includeMemoryAssets = false;
			filter.includeWithoutFilepath = false;
			g_assetManager->IterateAssetRegistryWithFilter(filter, [&](Volt::ReadOnlyAssetMetadata metadata) -> bool
			{
				Filesystem::Path filesystemPath = g_assetManager->GetAssetFilesystemPath(metadata->handle);
				pathToHandle[filesystemPath] = metadata->handle;
				return true;
			});

			for (auto& [dirID, dir] : m_directories)
			{
				for (auto& [assetID, assetItem] : dir.Assets)
				{
					if (pathToHandle.contains(assetItem.Path))
					{
						assetItem.Handle = pathToHandle[assetItem.Path];
						assetItem.Type = g_assetManager->GetReadOnlyAssetMetadata(assetItem.Handle)->type;
					}
				}
			}
		}

		//sort children
		{
			VT_PROFILE_SCOPE("SortEntries");

			for (auto& [dirID, dir] : m_directories)
			{
				std::sort(dir.Children.begin(), dir.Children.end(), [&](const AssetBrowserItemProxy& lhs, const AssetBrowserItemProxy& rhs)
				{
					//directories in the beginning
					if (lhs.Type == AssetBrowserItemType::Directory && rhs.Type == AssetBrowserItemType::Asset)
					{
						return true;
					}
					if (lhs.Type == AssetBrowserItemType::Asset && rhs.Type == AssetBrowserItemType::Directory)
					{
						return false;
					}

					VT_ENSURE(lhs.Type == rhs.Type);
					if (lhs.Type == AssetBrowserItemType::Directory)
					{
						const AssetBrowserDirectory& lhsDir = m_directories[lhs.ID];
						const AssetBrowserDirectory& rhsDir = m_directories[rhs.ID];
						return lhsDir.Path < rhsDir.Path;
					}

					if (lhs.Type == AssetBrowserItemType::Asset)
					{
						const AssetBrowserAssetItem& lhsAsset = dir.Assets[lhs.ID];
						const AssetBrowserAssetItem& rhsAsset = dir.Assets[rhs.ID];
						return lhsAsset.Path < rhsAsset.Path;
					}

					return false;
				});
			}
		}

		m_currentDirectoryID = m_directoryPathToID[baseDirectoryPath];
		AssetBrowserDirectory& baseDir = m_directories[m_currentDirectoryID];
		m_assetsListWidget->SetItemsSource(&baseDir.Children);


		bool expectedReloading = true;
		m_reloading.compare_exchange_strong(expectedReloading, false);
	};

	Volt::JobSystem::RunJob(Volt::JobSystem::CreateJob(
		"Reload Asset Browser...",
		Volt::ExecutionPriority::Latent,
		Volt::ExecutionPolicy::WorkerThread,
		nullptr,
		g_assetManager->GetMetadataLoadingCounter(),
		reloadFunc,
		Volt::FiberStackSize::KB32)
	);
}

glm::vec2 AssetBrowserWidget::GetDesiredSize()
{
	return { -1, -1 };
}

Ref<Circuit::IListViewRow<AssetBrowserItemProxy>> AssetBrowserWidget::GenerateRow(AssetBrowserItemProxy& item)
{
	Ref<Widget> content = nullptr;
	if (item.Type == AssetBrowserItemType::Directory)
	{
		AssetBrowserDirectory& dir = m_directories[item.ID];

		content = CreateWidget(Circuit::TextWidget)
			.Text("[Directory] " + dir.Path.Stem().ToString())
			.Size(20.f)
			.Color(CircuitColor(0xffffffff));
	}
	else if (item.Type == AssetBrowserItemType::Asset)
	{
		AssetBrowserDirectory& parentDir = m_directories[item.ParentDirectoryID];
		AssetBrowserAssetItem& asset = parentDir.Assets[item.ID];
		content = CreateWidget(Circuit::TextWidget)
			.Text("[Asset] " + asset.Path.Stem().ToString())
			.Size(20.f)
			.Color(CircuitColor(0xffffffff));
	}


	VT_ENSURE(content);
	auto row = CreateWidget(Circuit::IListViewRow<AssetBrowserItemProxy>)
		.Content(content);

	return row;
}

void AssetBrowserWidget::OnRowDoubleClicked(AssetBrowserItemProxy& item)
{
	if (item.Type != AssetBrowserItemType::Directory)
	{
		return;
	}

	m_currentDirectoryID = item.ID;
	AssetBrowserDirectory& baseDir = m_directories[m_currentDirectoryID];
	m_assetsListWidget->SetItemsSource(&baseDir.Children);
}

void AssetBrowserWidget::OnClickedReload(Volt::InputCode button)
{
	if (button != Volt::InputCode::Mouse_LB)
	{
		return;
	}

	RediscoverFiles();
}

void AssetBrowserWidget::OnClickedUp(Volt::InputCode button)
{
	if (button != Volt::InputCode::Mouse_LB)
	{
		return;
	}


	AssetBrowserDirectory& currDir = m_directories[m_currentDirectoryID];
	const Filesystem::Path& currPath = currDir.Path;
	Filesystem::Path parentPath = currPath.ParentPath();
	if (!m_directoryPathToID.contains(parentPath))
	{
		return;
	}

	m_currentDirectoryID = m_directoryPathToID[parentPath];
	AssetBrowserDirectory& parentDir = m_directories[m_currentDirectoryID];
	m_assetsListWidget->SetItemsSource(&parentDir.Children);
}
