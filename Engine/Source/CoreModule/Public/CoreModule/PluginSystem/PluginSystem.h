#pragma once

#include "CoreModule/Config.h"
#include "CoreModule/Plugin/Plugin.h"
#include "CoreModule/PluginSystem/NativePluginModule.h"

#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Containers/Graph.h>
#include <CoreUtilities/VoltGUID.h>

namespace Volt
{
	struct Project;
	struct PluginDefinition;
	class Plugin;
	class PluginRegistry;

	typedef Plugin* (*PFN_PluginCreateInstance)(const PluginInitializationInfo& info);
	typedef void(*PFN_PluginDestroyInstance)(Plugin* plugin);

	inline static constexpr const char* PLUGIN_CREATE_FUNC_NAME = "CreatePluginInstance";
	inline static constexpr const char* PLUGIN_DESTROY_FUNC_NAME = "DestroyPluginInstance";

	template<typename PluginType>
	struct PluginContainer
	{
		PluginType* pluginPtr;
		NativePluginModule nativePlugin;
		bool initialized = false;
	};

	class PluginFactory
	{
	public:
		VTC_API PluginFactory(PFN_PluginCreateInstance createFunc, PFN_PluginDestroyInstance destroyFunc);

		VTC_API Plugin* CreateInstance(const PluginInitializationInfo& info);
		VTC_API void DestroyInstance(Plugin* plugin);

	private:
		PFN_PluginCreateInstance m_createInstanceFunc;
		PFN_PluginDestroyInstance m_destroyInstanceFunc;
	};

	class PluginSystem : public SubSystem
	{
	public:
		~PluginSystem() override = default;

		void Initialize() override;
		void Shutdown() override;
		void OnPostInitialization() override;
		void OnPreShutdown() override;
		
		VTC_API void LoadPlugins(const Project& project);
		VTC_API void UnloadPlugins();

		static void GetSubSystemDependencies(SubSystemDependencyList& outDependencies);
		VT_DECLARE_SUBSYSTEM("{AC054603-4CED-42FD-8BC8-370713EB5EE4}"_guid)

	private:
		void InitializePlugins();
		void ShutdownPlugins();
		bool LoadPlugin(const PluginDefinition& pluginDefinition);
		
		void InitializePluginAndDependencies(UUID64 nodeId, const Graph<VoltGUID, uint32_t>& dependencyGraph);

		PluginRegistry* m_pluginRegistry = nullptr;

		Map<Filesystem::Path, Ref<PluginFactory>> m_pluginFactories;
		Map<VoltGUID, size_t> m_guidToIndexMap;

		Vector<PluginContainer<Plugin>> m_loadedPlugins;
	};
}
