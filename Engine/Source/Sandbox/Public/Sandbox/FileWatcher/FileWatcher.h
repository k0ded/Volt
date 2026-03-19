#pragma once

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Pointers/Unique.h>

#include <efsw/efsw.hpp>

class FileListener;
class FileWatcher
{
public:
	FileWatcher();
	~FileWatcher();

	void AddWatch(const std::filesystem::path& path, bool recursive = true);
	void AddCallback(efsw::Actions::Action action, std::function<void(const std::filesystem::path, const std::filesystem::path)>&& callback);

	inline static FileWatcher& Get() { return *myInstance; }
private:
	inline static FileWatcher* myInstance = nullptr;

	Unique<efsw::FileWatcher> myFileWatcher;
	Unique<FileListener> myFileListener;

	Vector<efsw::WatchID> myWatchIds;
};
