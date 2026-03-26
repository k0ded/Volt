#include "sbpch.h"
#include "FileWatcher/FileListener.h"

void FileListener::handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename)
{
	for (const auto& callback : myCallbacks[action])
	{
		const Filesystem::Path newPath = Filesystem::Path(StringView(dir.c_str(), dir.size()) / Filesystem::Path(StringView(filename.c_str(), filename.size())));
		const Filesystem::Path oldPath = StringView(oldFilename.c_str(), oldFilename.size());

		callback(newPath, oldPath);
	}
}

void FileListener::AddCallback(std::function<void(const Filesystem::Path, const Filesystem::Path)>&& callback, efsw::Actions::Action action)
{
	myCallbacks[action].emplace_back(std::move(callback));
}
