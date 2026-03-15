#include "vtcorepch.h"

#include "Volt-Core/PluginSystem/PluginRegistry.h"

#include <Volt-FileSystem/FileUtility.h>

#include <CoreUtilities/DynamicLibraryHelpers.h>
#include <CoreUtilities/FileSystem.h>
#include <CoreUtilities/JSON/JSONReader.h>

VT_DEFINE_LOG_CATEGORY(LogPluginSystem);

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(PluginRegistry, Default, PreEngine);

	constexpr std::string_view PLUGIN_EXTENSION = ".vtplugin";

	void PluginRegistry::OnPostStageInitializaton()
	{
		BuildPluginDependencies();
	}

	void PluginRegistry::FindAndRegisterPluginsInDirectory(const std::filesystem::path& directory)
	{
		if (!FileSystem::Exists(directory))
		{
			return;
		}

		VT_LOGC(Info, LogPluginSystem, "Starting registering of plugins in directory {}!", directory.string());

		for (const auto& path : std::filesystem::recursive_directory_iterator(directory))
		{
			if (path.is_directory())
			{
				continue;
			}

			const auto& filepath = path.path();
			if (filepath.extension().string() != PLUGIN_EXTENSION)
			{
				continue;
			}
			
			DeserializePlugin(filepath);
		}

		VT_LOGC(Info, LogPluginSystem, "Finished registering of plugins!");
	}

	const PluginDefinition& PluginRegistry::GetPluginDefinitionByName(const std::string& name) const
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

	void PluginRegistry::DeserializePlugin(const std::filesystem::path& filepath)
	{
		std::string jsonString;
		if (!FileUtility::ReadStringFromFile(filepath, jsonString))
		{
			VT_LOGC(Warning, LogPluginSystem, "Unable to open file {}!", filepath.string());
			return;
		}

		JSONReader jsonReader;
		
		if (!jsonReader.Parse(jsonString))
		{
			VT_LOGC(Warning, LogPluginSystem, "Plugin file {} is invalid!", filepath.string());
		}

		PluginDefinition newPlugin{};

		jsonReader.TryGet("Name", newPlugin.name);
		
		std::string guidString;
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
			VT_LOGC(Error, LogPluginSystem, "Plugin {} does not have a GUID defined!", filepath.string());
			return;
		}

		const std::filesystem::path filename = (filepath.stem().string() + VT_SHARED_LIBRARY_EXTENSION);

		const std::filesystem::path executableDirectory = FileSystem::GetExecutablePath().parent_path();
		const std::filesystem::path executablePluginBinaryPath = executableDirectory / filename;
		const std::filesystem::path pluginsDirBinaryPath = filepath.parent_path() / filename;

		if (!FileSystem::Exists(executablePluginBinaryPath) && !FileSystem::Exists(pluginsDirBinaryPath))
		{
			VT_LOGC(Error, LogPluginSystem, "Plugin {} does not have a binary at the correct location!", filepath.string());
			return;
		}

		if (FileSystem::Exists(executablePluginBinaryPath))
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
