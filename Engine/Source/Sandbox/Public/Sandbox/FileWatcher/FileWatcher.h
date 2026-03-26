#pragma once

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Pointers/Unique.h>
#include <CoreUtilities/Filesystem/Path.h>

#include <efsw/efsw.hpp>

class FileListener;
class FileWatcher
{
public:
	FileWatcher();
	~FileWatcher();

	void AddWatch(const Filesystem::Path& path, bool recursive = true);
	void AddCallback(efsw::Actions::Action action, std::function<void(const Filesystem::Path, const Filesystem::Path)>&& callback);

	inline static FileWatcher& Get() { return *myInstance; }
private:
	inline static FileWatcher* myInstance = nullptr;

	Unique<efsw::FileWatcher> myFileWatcher;
	Unique<FileListener> myFileListener;

	Vector<efsw::WatchID> myWatchIds;
};
