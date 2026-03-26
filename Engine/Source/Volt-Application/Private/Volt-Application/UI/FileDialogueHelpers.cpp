#include "vtapppch.h"

#include "Volt-Application/UI/FileDialogueHelpers.h"
#include <Volt-FileSystem/Filesystem.h>

#include <nfd.hpp>

namespace FileDialogueHelpers
{
	void Initialize()
	{
		NFD::Init();
	}

	void Shutdown()
	{
		NFD::Quit();
	}
	
	Filesystem::Path PickFolderDialogue(const Filesystem::Path& baseDir)
	{
		const auto absolutePath = Filesystem::Absolute(baseDir);

		NFD::UniquePath outPath;
		nfdresult_t result = NFD::PickFolder(outPath, absolutePath.ToString().c_str());

		switch (result)
		{
			case NFD_OKAY:
			{
				return outPath.get();
			}

			case NFD_ERROR:
			case NFD_CANCEL:
				break;
		}

		return "";
	}
	
	Filesystem::Path OpenFileDialogue(const Vector<FileFilter>& filters, const Filesystem::Path& baseDir)
	{
		Vector<nfdfilteritem_t> filterItems{};
		for (const auto& filter : filters)
		{
			filterItems.emplace_back(filter.name.c_str(), filter.extensions.c_str());
		}

		const auto absolutePath = Filesystem::Absolute(baseDir);

		NFD::UniquePath outPath;
		nfdresult_t result = NFD::OpenDialog(outPath, filterItems.data(), static_cast<nfdfiltersize_t>(filterItems.size()), absolutePath.ToString().c_str());

		switch (result)
		{
			case NFD_OKAY:
			{
				return outPath.get();
			}

			case NFD_ERROR:
			case NFD_CANCEL:
				break;
		}

		return "";
	}
	
	Filesystem::Path SaveFileDialogue(const Vector<FileFilter>& filters, const Filesystem::Path& baseDir)
	{
		Vector<nfdfilteritem_t> filterItems{};
		for (const auto& filter : filters)
		{
			filterItems.emplace_back(filter.name.c_str(), filter.extensions.c_str());
		}

		const auto absolutePath = Filesystem::Absolute(baseDir);

		NFD::UniquePath outPath;
		nfdresult_t result = NFD::SaveDialog(outPath, filterItems.data(), static_cast<nfdfiltersize_t>(filterItems.size()), absolutePath.ToString().c_str());

		switch (result)
		{
			case NFD_OKAY:
			{
				return outPath.get();
			}

			case NFD_ERROR:
			case NFD_CANCEL:
				break;
		}

		return "";
	}
}
