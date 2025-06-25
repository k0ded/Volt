#include "rhipch.h"
#include "RHIModule/RHIModuleLoader.h"

#include <EventSystem/ApplicationEvents.h>

#include <CoreUtilities/DynamicLibraryHelpers.h>
#include <CoreUtilities/StringUtility.h>

namespace Volt::RHI
{
	VT_REGISTER_SUBSYSTEM(RHIModuleLoader, Minimal, PreEngine, 2);

	RHIModuleLoader::RHIModuleLoader()
	{
		RegisterListener<AppPreRenderEvent>(VT_BIND_EVENT_FN(RHIModuleLoader::OnPreRenderEvent));
	}

	void RHIModuleLoader::LoadRHI(RHI::GraphicsAPI api, const RHI::RHICallbackInfo& callbackInfo)
	{
		// At this point in the runtime our working directory should be right out side of Binaries.
		// And for now we assume it is.
		std::filesystem::path filepath = "Binaries";
		switch (api)
		{
			case Volt::RHI::GraphicsAPI::Vulkan: filepath /= "VulkanRHIModule.dll"; break;
			case Volt::RHI::GraphicsAPI::D3D12: filepath /= "D3D12RHIModule.dll"; break;
		}

		VT_ENSURE_MSG(std::filesystem::exists(filepath), std::format("RHI module at filepath {} not found!", filepath));

		LoadRHIFromFilepath(filepath);
		CreateGraphicsContextForRHI(api, callbackInfo);
	}

	void RHIModuleLoader::LoadRHIFromFilepath(const std::filesystem::path& filepath)
	{
		const std::string cleanFilepath = ::Utility::ReplaceCharacter(filepath.string(), '/', '\\');

		// Check if module is already loaded
		m_rhiModuleHandle = VT_GET_MODULE_HANDLE(cleanFilepath.c_str());
		if (!m_rhiModuleHandle)
		{
			m_rhiModuleHandle = VT_LOAD_LIBRARY(cleanFilepath.c_str());
		}

		VT_ENSURE_MSG(m_rhiModuleHandle != nullptr, "RHI module failed to load!");

		PFN_CreateRHIModule createFunc = reinterpret_cast<PFN_CreateRHIModule>(VT_GET_PROC_ADDRESS(m_rhiModuleHandle, RHI_CREATE_FUNC_NAME));
		VT_ENSURE_MSG(createFunc != nullptr, "Could not find CreateRHIModule in RHI module!");

		m_rhiModule = createFunc();
	}

	void RHIModuleLoader::CreateGraphicsContextForRHI(RHI::GraphicsAPI api, const RHI::RHICallbackInfo& callbackInfo)
	{
		RHI::GraphicsContextCreateInfo createInfo;
		createInfo.graphicsApi = api;

		m_rhiModule->SetRHICallbackInfo(callbackInfo);
		m_graphicsContext = RHI::GraphicsContext::Create(createInfo);
	}

	void RHIModuleLoader::Shutdown()
	{
		if (m_rhiModuleHandle && m_rhiModule)
		{
			m_rhiModule->FlushResourceDeletionQueue();
			m_graphicsContext = nullptr;
		
			PFN_DestroyRHIModule destroyFunc = reinterpret_cast<PFN_DestroyRHIModule>(VT_GET_PROC_ADDRESS(m_rhiModuleHandle, RHI_DESTROY_FUNC_NAME));
			VT_ENSURE_MSG(destroyFunc != nullptr, "Could not find DestroyRHIModule in RHI module!");

			destroyFunc(m_rhiModule);
			m_rhiModule = nullptr;
		}
	}

	bool RHIModuleLoader::OnPreRenderEvent(AppPreRenderEvent& event)
	{
		m_rhiModule->Update();
		return false;
	}
}
