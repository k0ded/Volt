#pragma once

#include "CoreModule/PluginSystem/PluginDefinition.h"
#include "CoreModule/Version.h"

#include <filesystem>

namespace Volt
{
	struct Project
	{
		Version engineVersion;
		String name;
		String companyName;

		Filesystem::Path filepath; // Filepath to the .vtproj file
		Filesystem::Path rootDirectory; // The directory containing the .vtproj file
		String assetsDirectoryName;
		Filesystem::Path audioDirectory; // The directory containing the audio banks, relative to the rootDirectory

		Filesystem::Path cursorFilepath; // Filepath to the cursor that the project should use when built
		Filesystem::Path iconFilepath; // Filepath to the icon that the project should use when built;
		Filesystem::Path startSceneFilepath; // Filepath to the scene that the project should start with when built;

		Vector<PluginDefinition> pluginDefinitions;

		bool isDeprecated = false;

		// Runtime values
		Filesystem::Path assetsDirectory;
		Filesystem::Path generatedDirectory;
	};
}
