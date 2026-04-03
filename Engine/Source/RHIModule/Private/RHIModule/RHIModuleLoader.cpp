#include "rhipch.h"

#include "RHIModule/RHIModuleLoader.h"
#include "RHIModule/Graphics/DeviceQueue.h"

#include <EventSystem/ApplicationEvents.h>
#include <EventSystem/EventSystem.h>

#include <FileSystemModule/Filesystem.h>
#include <FileSystemModule/IOThreads/IOThreads.h>

#include <CoreModule/GlobalCommandLine.h>
#include <CoreModule/Project/ProjectManager.h>

#include <CoreUtilities/DynamicLibraryHelpers.h>
#include <CoreUtilities/String/StringUtility.h>

namespace Volt::RHI
{
	VT_REGISTER_SUBSYSTEM(RHIModuleLoader, Minimal, PreEngine);

	RHIModuleLoader::RHIModuleLoader()
	{
	}

	void RHIModuleLoader::LoadRHI(const RHIConfig& rhiConfig, const RHI::RHICallbackInfo& callbackInfo)
	{
		// At this point in the runtime our working directory should be right out side of Binaries.
		// And for now we assume it is.
		Filesystem::Path filepath = "Binaries";
		switch (rhiConfig.api)
		{
			case Volt::RHI::GraphicsAPI::Vulkan: filepath /= "VulkanRHIModule.dll"; break;
			case Volt::RHI::GraphicsAPI::D3D12: filepath /= "D3D12RHIModule.dll"; break;
		}

		VT_ENSURE_MSG(Filesystem::Exists(filepath), FormatString("RHI module at filepath {} not found!", filepath));

		LoadRHIFromFilepath(filepath);
		CreateGraphicsContextForRHI(rhiConfig, callbackInfo);
	}

	void RHIModuleLoader::LoadRHIFromFilepath(const Filesystem::Path& filepath)
	{
		Filesystem::Path tempPath = filepath;
		tempPath.MakePreferred();

		// Check if module is already loaded
		m_rhiModuleHandle = VT_GET_MODULE_HANDLE(tempPath.CStr());
		if (!m_rhiModuleHandle)
		{
			m_rhiModuleHandle = VT_LOAD_LIBRARY(tempPath.CStr());
		}

		VT_ENSURE_MSG(m_rhiModuleHandle != nullptr, "RHI module failed to load!");

		PFN_CreateRHIModule createFunc = reinterpret_cast<PFN_CreateRHIModule>(VT_GET_PROC_ADDRESS(m_rhiModuleHandle, RHI_CREATE_FUNC_NAME));
		VT_ENSURE_MSG(createFunc != nullptr, "Could not find CreateRHIModule in RHI module!");

		m_rhiModule = createFunc();
	}

	void RHIModuleLoader::CreateGraphicsContextForRHI(const RHIConfig& rhiConfig, const RHI::RHICallbackInfo& callbackInfo)
	{
		RHI::GraphicsContextCreateInfo createInfo;
		createInfo.graphicsApi = rhiConfig.api;
		createInfo.enableDebugLayer = rhiConfig.enableDebugLayer;
		createInfo.pipelineCacheFilepath = rhiConfig.pipelineCacheFilepath;

		m_rhiModule->SetRHICallbackInfo(callbackInfo);
		m_graphicsContext = RHI::GraphicsContext::Create(createInfo);
	}

	void RHIModuleLoader::Initialize()
	{
		RHI::RHICallbackInfo callbackInfo{};

		RHI::RHIConfig rhiConfig;
		rhiConfig.api = RHI::GraphicsAPI::Vulkan;
		rhiConfig.enableDebugLayer = false;
		rhiConfig.pipelineCacheFilepath = ProjectManager::GetProjectDirectory() / "Generated" / "PipelineCache.bin";

		const CommandLineBuilder& commandLineBuilder = GlobalCommandLine::Get();

		if (commandLineBuilder.IsArgDefined("vulkan"))
		{
			rhiConfig.api = RHI::GraphicsAPI::Vulkan;
		}
		
		if (commandLineBuilder.IsArgDefined("d3d12"))
		{
			rhiConfig.api = RHI::GraphicsAPI::D3D12;
		}

		if (commandLineBuilder.IsArgDefined("rhidebuglayer"))
		{
			rhiConfig.enableDebugLayer = true;
		}

		LoadRHI(rhiConfig, callbackInfo);
	}

	void RHIModuleLoader::Shutdown()
	{
		if (m_rhiModuleHandle && m_rhiModule)
		{
			m_graphicsContext->GetDevice()->GetDeviceQueue(QueueType::Graphics)->WaitForQueue();
			m_graphicsContext->GetDevice()->GetDeviceQueue(QueueType::Compute)->WaitForQueue();
			m_graphicsContext->GetDevice()->GetDeviceQueue(QueueType::TransferCopy)->WaitForQueue();

			m_rhiModule->FlushResourceDeletionQueue();
			m_graphicsContext = nullptr;
		
			PFN_DestroyRHIModule destroyFunc = reinterpret_cast<PFN_DestroyRHIModule>(VT_GET_PROC_ADDRESS(m_rhiModuleHandle, RHI_DESTROY_FUNC_NAME));
			VT_ENSURE_MSG(destroyFunc != nullptr, "Could not find DestroyRHIModule in RHI module!");

			destroyFunc(m_rhiModule);
			m_rhiModule = nullptr;
		}
	}

	void RHIModuleLoader::GetSubSystemDependencies(SubSystemDependencyList& outDependencies)
	{
		outDependencies.AddDependency<IOThreads>();
		outDependencies.AddDependency<ProjectManager>();
	}

	void RHIModuleLoader::OnPreRender()
	{
		m_rhiModule->BeginFrame();
	}

	void RHIModuleLoader::OnPostRender()
	{
		m_rhiModule->EndFrame();
	}
}
