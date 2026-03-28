#include "cpch.h"
#include "CoreModule/PluginSystem/NativePluginModule.h"
#include "CoreModule/DynamicLibraryManager.h"

namespace Volt
{
	NativePluginModule::NativePluginModule(const Filesystem::Path& binaryFilepath, bool externallyLoaded) noexcept
		: m_binaryFilepath(binaryFilepath), m_externallyLoaded(externallyLoaded)
	{
	}

	NativePluginModule::NativePluginModule(const NativePluginModule& other) noexcept
	{
		m_binaryFilepath = other.m_binaryFilepath;
		m_externallyLoaded = other.m_externallyLoaded;
	}

	NativePluginModule& NativePluginModule::operator=(const NativePluginModule& other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}

		m_binaryFilepath = other.m_binaryFilepath;
		m_externallyLoaded = other.m_externallyLoaded;

		return *this;
	}

	NativePluginModule::NativePluginModule(NativePluginModule&& other) noexcept
		: m_binaryFilepath(other.m_binaryFilepath), m_externallyLoaded(other.m_externallyLoaded)
	{
		other.m_binaryFilepath.Clear();
	}

	NativePluginModule::~NativePluginModule()
	{
		if (!m_binaryFilepath.IsEmpty() && !m_externallyLoaded)
		{
			DynamicLibraryManager::Get().UnloadDynamicLibrary(m_binaryFilepath);
		}
	}

	void NativePluginModule::Unload()
	{
		if (!m_binaryFilepath.IsEmpty() && !m_externallyLoaded)
		{
			DynamicLibraryManager::Get().UnloadDynamicLibrary(m_binaryFilepath);
		}

		m_binaryFilepath.Clear();
	}
}
