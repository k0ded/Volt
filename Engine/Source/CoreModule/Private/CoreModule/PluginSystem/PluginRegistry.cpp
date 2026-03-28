#include "cpch.h"
#include "CoreModule/PluginSystem/PluginRegistry.h"

#include <FileSystemModule/FileUtility.h>

#include <CoreUtilities/DynamicLibraryHelpers.h>

#include <FileSystemModule/Filesystem.h>
#include <FileSystemModule/Iterators/RecursiveDirectoryIterator.h>

#include <CoreModule/JSON/JSONReader.h>

VT_DEFINE_LOG_CATEGORY(LogPluginSystem);

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(PluginRegistry, Default, PreEngine);

	constexpr StringView PLUGIN_EXTENSION = ".vtplugin";

	void PluginRegistry::OnPostStageInitializaton()
	{
		BuildPluginDependencies();
	}

	void PluginRegistry::FindAndRegisterPluginsInDirectory(const Filesystem::Path& directory)
	{
		if (!Filesystem::Exists(directory))
		{
			return;
		}

		VT_LOGC(Info, LogPluginSystem, "Starting registering of plugins in directory {}!", directory);

		for (const auto& entry : Filesystem::RecursiveDirectoryIterator(directory))
		{
			if (entry.isDirectory)
			{
				continue;
			}

			const Filesystem::Path& filepath = entry.path;
			if (filepath.Extension() != PLUGIN_EXTENSION)
			{
				continue;
			}

			DeserializePlugin(filepath);
		}

		VT_LOGC(Info, LogPluginSystem, "Finished registering of plugins!");
	}

	const PluginDefinition& PluginRegistry::GetPluginDefinitionByName(const String& name) const
	{
		for (const auto& [guid, definition] : m_registeredPlugins)
		{
			if (definition.name == name)
			{
				return definition;
			}
		}

		static PluginDefinition nullDefinition;
		return nullDefinition;
	}

	void PluginRegistry::BuildPluginDependencies()
	{
		for (const auto& [guid, pluginDefinition] : m_registeredPlugins)
		{
			if (!m_guidToNodeId.contains(guid))
			{
				m_guidToNodeId[guid] = m_pluginDependencyGraph.AddNode(guid);
			}

			for (const auto& depName : pluginDefinition.pluginDependencies)
			{
				const auto& depDefinition = GetPluginDefinitionByName(depName);
				if (!depDefinition.IsValid())
				{
					continue;
				}

				if (!m_guidToNodeId.contains(depDefinition.guid))
				{
					m_guidToNodeId[guid] = m_pluginDependencyGraph.AddNode(depDefinition.guid);
				}

				m_pluginDependencyGraph.LinkNodes(m_guidToNodeId.at(guid), m_guidToNodeId.at(depDefinition.guid));
			}
		}
	}

	void PluginRegistry::DeserializePlugin(const Filesystem::Path& filepath)
	{
		String jsonString;
		if (!FileUtility::ReadStringFromFile(filepath, jsonString))
		{
			VT_LOGC(Warning, LogPluginSystem, "Unable to open file {}!", filepath);
			return;
		}

		JSONReader jsonReader;
		
		if (!jsonReader.Parse(jsonString))
		{
			VT_LOGC(Warning, LogPluginSystem, "Plugin file {} is invalid!", filepath);
		}

		PluginDefinition newPlugin{};

		jsonReader.TryGet("Name", newPlugin.name);
		
		String guidString;
		if (jsonReader.TryGet("GUID", guidString))
		{
			if (guidString.empty())
			{
				newPlugin.guid = VoltGUID::Null();
			}
			else
			{
				newPlugin.guid = VoltGUID::FromStringInternal(guidString.c_str());
			}
		}

		if (newPlugin.guid == VoltGUID::Null())
		{
			VT_LOGC(Error, LogPluginSystem, "Plugin {} does not have a GUID defined!", filepath);
			return;
		}

		const Filesystem::Path filename = (filepath.Stem() + VT_SHARED_LIBRARY_EXTENSION);

		const Filesystem::Path executableDirectory = Filesystem::GetExecutablePath().ParentPath();
		const Filesystem::Path executablePluginBinaryPath = executableDirectory / filename;
		const Filesystem::Path pluginsDirBinaryPath = filepath.ParentPath() / filename;

		if (!Filesystem::Exists(executablePluginBinaryPath) && !Filesystem::Exists(pluginsDirBinaryPath))
		{
			VT_LOGC(Error, LogPluginSystem, "Plugin {} does not have a binary at the correct location!", filepath);
			return;
		}

		if (Filesystem::Exists(executablePluginBinaryPath))
		{
			newPlugin.binaryFilepath = executablePluginBinaryPath;
		}
		else
		{
			newPlugin.binaryFilepath = pluginsDirBinaryPath;
		}

		jsonReader.IterateArray("Plugins", [&]() 
		{
			jsonReader.Get(newPlugin.pluginDependencies.emplace_back());
		});

		m_registeredPlugins[newPlugin.guid] = newPlugin;
		VT_LOGC(Info, LogPluginSystem, "Plugin {} has been registered!", newPlugin.name);
	}
}
