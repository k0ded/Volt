#include "vtapppch.h"

#include "Volt-Application/UIApplication.h"

#include <Volt-Renderer/Renderer.h>

#include <WindowModule/WindowManager.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <VulkanRHIModule/VulkanRHIProxy.h>
#include <D3D12RHIModule/D3D12RHIProxy.h>

#include <CoreUtilities/Allocator.h>
#include <CoreUtilities/Allocators/PagedHeapAllocator.h>
#include <CoreUtilities/FileSystem.h>

namespace Volt
{
	UIApplication::UIApplication(const CommandLineBuilder& commandLineBuilder, const ApplicationCreationInfo& createInfo)
		: BaseApplication(commandLineBuilder, createInfo)
	{
		g_heapAllocator = CreateScope<PagedHeapAllocator>();
	
		FileSystem::Initialize();
		FileSystem::InitializeWorkingDirectory(createInfo.isRuntime, commandLineBuilder);

		m_subSystemManager = CreateScope<SubSystemManager>(SubSystemInclusionLevel::Minimal);
		m_subSystemManager->InitializeSubSystems(SubSystemInitializationStage::PreEngine);

		// This is required because glfwInit must be called before setting up graphics device
		WindowManager::InitializeGLFW();
		CreateGraphicsContext();

		m_subSystemManager->InitializeSubSystems(SubSystemInitializationStage::Engine);
		m_subSystemManager->InitializeSubSystems(SubSystemInitializationStage::PostEngine);
	}
	
	UIApplication::~UIApplication()
	{
		m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::PostEngine);
		m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::Engine);

		m_graphicsContext = nullptr;
		m_rhiProxy = nullptr;
		WindowManager::ShutdownGLFW();

		m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::PreEngine);

		m_subSystemManager = nullptr;

		FileSystem::Shutdown();

		g_heapAllocator.reset();
	}
	
	void UIApplication::Run()
	{
		VT_PROFILE_THREAD("Main");

		m_isRunning = true;

		while (m_isRunning)
		{

		}
	}

	void UIApplication::Quit()
	{
		m_isRunning = false;
	}

	void UIApplication::PushLayer(ApplicationLayer* layer)
	{
	}

	void UIApplication::PopLayer(ApplicationLayer* layer)
	{

	}

	void UIApplication::CreateGraphicsContext()
	{
		RHI::GraphicsContextCreateInfo cinfo{};
		cinfo.graphicsApi = RHI::GraphicsAPI::Vulkan;

		if (cinfo.graphicsApi == RHI::GraphicsAPI::Vulkan)
		{
			m_rhiProxy = RHI::CreateVulkanRHIProxy();
		}
		else if (cinfo.graphicsApi == RHI::GraphicsAPI::D3D12)
		{
			m_rhiProxy = RHI::CreateD3D12RHIProxy();
		}

		{
			RHI::RHICallbackInfo callbackInfo{};
			callbackInfo.resourceManagementInfo.resourceDeletionCallback = Renderer::DestroyResource;
			//callbackInfo.requestCloseEventCallback = []()
			//{
			//	WindowCloseEvent closeEvent{};
			//	EventSystem::DispatchEvent(closeEvent);
			//};

			m_rhiProxy->SetRHICallbackInfo(callbackInfo);
		}

		m_graphicsContext = RHI::GraphicsContext::Create(cinfo);
	}

	void UIApplication::MainUpdate()
	{
		WindowManager::Get().BeginFrame();

		WindowManager::Get().Present();
	}
}
