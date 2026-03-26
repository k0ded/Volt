#include "sbpch.h"
#include "FileWatcher/FileWatcher.h"

#include "Sandbox/FileWatcher/FileListener.h"

#include <CoreUtilities/String/StringUtility.h>

FileWatcher::FileWatcher()
{
	VT_ASSERT_MSG(!myInstance, "Instance already exists!");
	myInstance = this;

	myFileWatcher = CreateUnique<efsw::FileWatcher>();
	myFileListener = CreateUnique<FileListener>();

	myFileWatcher->watch();
}

FileWatcher::~FileWatcher()
{
	for (const auto& id : myWatchIds)
	{
		myFileWatcher->removeWatch(id);
	}

	myWatchIds.clear();
	myFileWatcher = nullptr;
	myFileListener = nullptr;

	myInstance = nullptr;
}

void FileWatcher::AddWatch(const Filesystem::Path& path, bool recursive)
{
	String watchPath = Utility::ReplaceCharacter(path.ToString(), '\\', '/');

	std::string tempStr(watchPath.begin(), watchPath.end());

	efsw::WatchID watchId = myFileWatcher->addWatch(tempStr, myFileListener.GetRaw(), recursive);
	myWatchIds.emplace_back(watchId);
}

void FileWatcher::AddCallback(efsw::Actions::Action action, std::function<void(const Filesystem::Path, const Filesystem::Path)>&& callback)
{
	myFileListener->AddCallback(std::move(callback), action);
}
