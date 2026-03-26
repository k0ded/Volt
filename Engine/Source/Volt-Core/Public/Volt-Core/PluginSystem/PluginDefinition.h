#pragma once

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/VoltGUID.h>

#include <CoreUtilities/Filesystem/Path.h>

namespace Volt
{
	struct PluginDefinition
	{
		String name;
		Filesystem::Path filepath; // Filepath to the plugin description file
		Filesystem::Path binaryFilepath; // Filepath to the plugin binary file
		VoltGUID guid;

		Vector<String> pluginDependencies;
	
		VT_INLINE bool IsValid() const
		{
			return guid != VoltGUID::Null();
		}
	};
}
