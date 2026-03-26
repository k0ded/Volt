#pragma once

#include "Volt-Core/PluginSystem/PluginDefinition.h"
#include "Volt-Core/Config.h"

#include <LogModule/LogCategory.h>
#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <CoreUtilities/VoltGUID.h>
#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Containers/Graph.h>

VT_DECLARE_LOG_CATEGORY(LogPluginSystem, LogVerbosity::Trace);

namespace Volt
{
	class VTCORE_API PluginRegistry : public SubSystem
	{
	public:
		~PluginRegistry() override = default;

		void OnPostStageInitializaton() override;

		void FindAndRegisterPluginsInDirectory(const Filesystem::Path& directory);
		VT_NODISCARD const PluginDefinition& GetPluginDefinitionByName(const String& name) const;

		void BuildPluginDependencies();
		VT_NODISCARD VT_INLINE const Graph<VoltGUID, uint32_t>& GetPluginDependencyGraph() const { return m_pluginDependencyGraph; }

		VT_DECLARE_SUBSYSTEM("{DA658B2C-1C38-433F-B2C4-62958E996E6E}"_guid)

	private:
		void DeserializePlugin(const Filesystem::Path& filepath);

		Map<VoltGUID, PluginDefinition> m_registeredPlugins;

		Graph<VoltGUID, uint32_t> m_pluginDependencyGraph;
		Map<VoltGUID, UUID64> m_guidToNodeId;
	};
}
