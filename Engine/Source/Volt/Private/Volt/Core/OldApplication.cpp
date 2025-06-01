#include "vtpch.h"
#include "Volt/Core/OldApplication.h"

#include "Volt/Steam/SteamImplementation.h"
#include "Volt/Utility/Noise.h"

//#include <Volt-Application/ImGuiSubsystem.h>
//#include <Volt-Application/UIUtility.h>

#include <Volt-Renderer/Renderer.h>

#include <Volt-Scene/SceneManager.h>

#include <Volt-Core/PluginSystem/PluginRegistry.h>
#include <Volt-Core/PluginSystem/PluginSystem.h>
#include <Volt-Core/Layer/Layer.h>

#include <Volt-Physics/PhysicsSubSystem.h>

#include <Volt-Platforms/Platform.h>

#include <RenderCore/RenderGraph/RenderGraphExecutionThread.h>

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetSerializerRegistry.h>
#include <AssetSystem/AssetFactory.h>

#include <RHIModule/ImGui/ImGuiImplementation.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/FrameCapture.h>

#include <VulkanRHIModule/VulkanRHIProxy.h>
#include <D3D12RHIModule/D3D12RHIProxy.h>

#include <Amp/WWiseEngine/WWiseEngine.h>
#include <Navigation/Core/NavigationSystem.h>

#include <LogModule/Log.h>

#include <InputModule/Events/KeyboardEvents.h>

#include <WindowModule/Events/WindowEvents.h>
#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

#include <EventSystem/EventSystem.h>
#include <EventSystem/ApplicationEvents.h>

#include <CoreUtilities/FileSystem.h>
#include <CoreUtilities/Allocator.h>

namespace Volt
{
	ApplicationEventListener::ApplicationEventListener(Application& application)
		: m_application(application)
	{ 
		RegisterListener<AppUpdateEvent>(VT_BIND_EVENT_FN(ApplicationEventListener::OnAppUpdateEvent));
		RegisterListener<WindowCloseEvent>(VT_BIND_EVENT_FN(ApplicationEventListener::OnWindowCloseEvent));
		RegisterListener<WindowResizeEvent>(VT_BIND_EVENT_FN(ApplicationEventListener::OnWindowResizeEvent));
		RegisterListener<ViewportResizeEvent>(VT_BIND_EVENT_FN(ApplicationEventListener::OnViewportResizeEvent));
		RegisterListener<KeyPressedEvent>(VT_BIND_EVENT_FN(ApplicationEventListener::OnKeyPressedEvent));
	}

	bool ApplicationEventListener::OnAppUpdateEvent(AppUpdateEvent& e)
	{
		return m_application.OnAppUpdateEvent(e);
	}

	bool ApplicationEventListener::OnWindowCloseEvent(WindowCloseEvent& e)
	{
		return m_application.OnWindowCloseEvent(e);
	}

	bool ApplicationEventListener::OnWindowResizeEvent(WindowResizeEvent& e)
	{
		return m_application.OnWindowResizeEvent(e);
	}

	bool ApplicationEventListener::OnViewportResizeEvent(ViewportResizeEvent& e)
	{
		return m_application.OnViewportResizeEvent(e);
	}

	bool ApplicationEventListener::OnKeyPressedEvent(KeyPressedEvent& e)
	{
		return m_application.OnKeyPressedEvent(e);
	}

	Application::Application(const ApplicationInfo& info, const CommandLineBuilder& commandLineBuilder)
		: m_frameTimer(100), m_info(info), m_commandLineBuilder(commandLineBuilder)
	{
		VT_ASSERT_MSG(!s_instance, "Application already exists!");
		s_instance = this;

		g_heapAllocator = CreateScope<PagedHeapAllocator>();

		FileSystem::Initialize();
		FileSystem::InitializeWorkingDirectory(info.isRuntime, commandLineBuilder);

		m_subSystemManager = CreateScope<SubSystemManager>();
		m_subSystemManager->InitializeSubSystems(SubSystemInitializationStage::PreEngine);

		m_logSubSystem = SubSystemManager::GetSubSystem<Log>();
		m_logSubSystem->EnableLogging(m_info.enableLogging);

		m_pluginSystem = SubSystemManager::GetSubSystem<PluginSystem>();
		m_pluginRegistry = SubSystemManager::GetSubSystem<PluginRegistry>();
		m_projectManager = SubSystemManager::GetSubSystem<ProjectManager>();

		m_pluginSystem->SetPluginRegistry(m_pluginRegistry);

		std::filesystem::path projectFilepath;
		if (m_commandLineBuilder.IsArgDefined("project"))
		{
			projectFilepath = m_commandLineBuilder.GetArgValue("project");
		}

		m_projectManager->LoadProject(projectFilepath, *m_pluginRegistry);
		m_pluginRegistry->BuildPluginDependencies();
		m_pluginSystem->LoadPlugins(ProjectManager::GetProject());

		// This is required because glfwInit must be called before setting up graphics device
		WindowManager::InitializeGLFW();
		CreateGraphicsContext();

		m_assetManager = CreateScope<AssetManager>(ProjectManager::GetRootDirectory(), ProjectManager::GetAssetsDirectory(), ProjectManager::GetEngineDirectory());
		m_sourceAssetManager = CreateScope<SourceAssetManager>();

		m_windowManager = SubSystemManager::GetSubSystem<WindowManager>();

		if (m_info.createMainWindow)
		{
			WindowProperties windowProperties{};
			windowProperties.Width = info.width;
			windowProperties.Height = info.height;
			windowProperties.VSync = info.useVSync;
			windowProperties.Title = info.title;
			windowProperties.WindowMode = info.windowMode;
			windowProperties.IconPath = info.iconPath;
			windowProperties.CursorPath = info.cursorPath;
			windowProperties.UseTitlebar = info.useTitlebar;
			windowProperties.UseCustomTitlebar = info.useCustomTitlebar;

			if (m_info.isRuntime)
			{
				windowProperties.Title = ProjectManager::GetProject().name;
				windowProperties.CursorPath = ProjectManager::GetProject().cursorFilepath;
				windowProperties.IconPath = ProjectManager::GetProject().iconFilepath;
			}

			if (ProjectManager::GetProject().isDeprecated)
			{
				windowProperties.UseTitlebar = true;
			}

			m_windowManager->CreateMainWindow(windowProperties);
		}

		m_subSystemManager->InitializeSubSystems(SubSystemInitializationStage::Engine);
		m_physicsSubSystem = SubSystemManager::GetSubSystem<PhysicsSubSystem>();

		//Init AudioEngine
		{
			//std::filesystem::path defaultPath = ProjectManager::GetAudioBanksDirectory();
			//Amp::WWiseEngine::Get().InitWWise(defaultPath.c_str());
			//if (FileSystem::Exists(defaultPath))
			//{
			//	for (auto bankFile : std::filesystem::directory_iterator(ProjectManager::GetAudioBanksDirectory()))
			//	{
			//		if (bankFile.path().extension() == L".bnk")
			//		{
			//			Amp::WWiseEngine::Get().LoadBank(bankFile.path().filename().string().c_str());
			//		}
			//	}
			//}
		}

		m_navigationSystem = CreateScope<Volt::AI::NavigationSystem>();

		// Extras

		if (info.enableSteam)
		{
			m_steamImplementation = SteamImplementation::Create();
		}

		m_subSystemManager->InitializeSubSystems(SubSystemInitializationStage::PostEngine);

		//m_imguiSubSystem = SubSystemManager::GetSubSystem<ImGuiSubSystem>();
		//// Make sure that the main window exits, it is required to initialize ImGui.
		//if (m_info.createMainWindow && m_info.enableImGui)
		//{
		//	m_imguiSubSystem->InitializeImGui(m_info.enableImGuiViewports);
		//	m_imguiSubSystem->SetupContext();
		//}

		m_scriptingSystem = CreateScope<ScriptingSystem>();

		m_pluginSystem->InitializePlugins();
		m_eventListener = CreateScope<ApplicationEventListener>(*this);

		SetupFrameCapture();
	}


	Application::~Application()
	{
		m_eventListener = nullptr;
		m_pluginSystem->ShutdownPlugins();

		m_scriptingSystem = nullptr;

		m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::PostEngine);

		m_navigationSystem = nullptr;
		m_layerStack.Clear();
		SceneManager::Shutdown();

		//Amp::WWiseEngine::Get().TermWwise();

		m_assetManager->Clear();

		m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::Engine);

		m_assetManager = nullptr;
		g_assetSerializerRegistry.Clear();
		g_assetFactory.Clear();

		m_windowManager->DestroyMainWindow();

		m_graphicsContext = nullptr;
		m_rhiProxy = nullptr;
		WindowManager::ShutdownGLFW();

		m_pluginSystem->UnloadPlugins();
		m_pluginSystem = nullptr;
		m_pluginRegistry = nullptr;
		m_projectManager = nullptr;

		m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::PreEngine);
		 
		FileSystem::Shutdown();

		m_subSystemManager = nullptr;

		g_heapAllocator.reset();
		s_instance = nullptr;
	}

	void Application::Run()
	{
		VT_PROFILE_THREAD("Main");

		m_isRunning = true;

		while (m_isRunning)
		{
			VT_PROFILE_FRAME("Frame");
			MainUpdate();

			m_frameIndex++;
		}
	}

	void Application::Quit()
	{
		m_isRunning = false;
	}

	void Application::PushLayer(Layer* layer)
	{
		m_layerStack.PushLayer(layer);
	}

	void Application::PopLayer(Layer* layer)
	{
		m_layerStack.PopLayer(layer);
	}

	void Application::LaunchMainWindow()
	{
		if (!m_info.createMainWindow && !m_windowManager->HasMainWindow())
		{
			WindowProperties windowProperties{};
			windowProperties.Width = m_info.width;
			windowProperties.Height = m_info.height;
			windowProperties.VSync = m_info.useVSync;
			windowProperties.Title = m_info.title;
			windowProperties.WindowMode = m_info.windowMode;
			windowProperties.IconPath = m_info.iconPath;
			windowProperties.CursorPath = m_info.cursorPath;
			windowProperties.UseTitlebar = m_info.useTitlebar;
			windowProperties.UseCustomTitlebar = m_info.useCustomTitlebar;

			if (m_info.isRuntime)
			{
				windowProperties.Title = ProjectManager::GetProject().name;
				windowProperties.CursorPath = ProjectManager::GetProject().cursorFilepath;
				windowProperties.IconPath = ProjectManager::GetProject().iconFilepath;
			}

			if (ProjectManager::GetProject().isDeprecated)
			{
				windowProperties.UseTitlebar = true;
			}

			m_windowManager->CreateMainWindow(windowProperties);

			if (m_imguiSubSystem)
			{
				/*m_imguiSubSystem->InitializeImGui(m_info.enableImGuiViewports);
				m_imguiSubSystem->SetupContext();*/
			}

			m_skipPresentThisFrame = true;
		}
	}

	void Application::InitializeMainThread()
	{
		PlatformThread::AssignThreadToCore(PlatformThread::GetCurrentThreadHandle(), 0);
	}

	void Application::MainUpdate()
	{
		m_hasSentMouseMovedEvent = false;

		RHI::GraphicsContext::Update();

		WindowManager::Get().BeginFrame();

		m_currentDeltaTime = m_frameTimer.GetDeltaTime();
		m_frameTimer.Update();

		{
			VT_PROFILE_SCOPE("Application::Render");

			AppPreRenderEvent preRenderEvent;
			EventSystem::DispatchEvent(preRenderEvent);

			AppRenderEvent renderEvent(m_currentDeltaTime);
			EventSystem::DispatchEvent(renderEvent);

			m_windowManager->Render(m_currentDeltaTime);
		}

		{
			VT_PROFILE_SCOPE("Application::Update");

			AppUpdateEvent updateEvent(m_currentDeltaTime); 
			EventSystem::DispatchEvent(updateEvent);

			AssetManager::Update();
		}

		{
			//VT_PROFILE_SCOPE("Application::UpdateAudio");
			//Amp::WWiseEngine::Get().Update();
		}

		//if (m_info.enableImGui && m_imguiSubSystem->IsInitialized() && !m_skipPresentThisFrame)
		//{
		//	VT_PROFILE_SCOPE("Application::ImGui");

		//	m_imguiSubSystem->Begin();

		//	AppImGuiUpdateEvent imguiEvent{};
		//	EventSystem::DispatchEvent(imguiEvent);

		//	// #TODO_Ivar: HACK! Will keep this here for now. We need to make sure that the scene renderer output image is ready. 
		//	RenderGraphExecutionThread::WaitForFinishedExecution();
		//	m_imguiSubSystem->End();
		//}
		//else
		//{
		//	RenderGraphExecutionThread::WaitForFinishedExecution();
		//}

		{
			VT_PROFILE_SCOPE("Application::PostFrameUpdate");
			AppPostFrameUpdateEvent postFrameUpdateEvent{ m_currentDeltaTime };
			EventSystem::DispatchEvent(postFrameUpdateEvent);
		}

		if (!m_skipPresentThisFrame)
		{
			WindowManager::Get().Present();
		}
		m_skipPresentThisFrame = false;

		m_frameTimer.Accumulate();
	}

	void Application::CreateGraphicsContext()
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
			callbackInfo.requestCloseEventCallback = []()
			{
				WindowCloseEvent closeEvent{};
				EventSystem::DispatchEvent(closeEvent);
			};

			m_rhiProxy->SetRHICallbackInfo(callbackInfo);
		}

		m_graphicsContext = RHI::GraphicsContext::Create(cinfo);
	}

	void Application::SetupFrameCapture()
	{
		if (RHI::RHIProxy::GetInstance().GetFrameCapture())
		{
			RHI::RHIProxy::GetInstance().GetFrameCapture()->SetFlags(RHI::FrameCaptureFlags::DisableOverlay);
			RHI::RHIProxy::GetInstance().GetFrameCapture()->SetCaptureFileTargetFilePath(ProjectManager::GetProjectDirectory() / ("Volt-" + ProjectManager::GetProject().name));
		}
	}

	bool Application::OnAppUpdateEvent(AppUpdateEvent&)
	{
		if (m_steamImplementation)
		{
			m_steamImplementation->Update();
		}
		return false;
	}

	bool Application::OnWindowCloseEvent(WindowCloseEvent&)
	{
		m_isRunning = false;
		return false;
	}

	bool Application::OnWindowResizeEvent(WindowResizeEvent& e)
	{
		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_isMinimized = true;
		}
		else
		{
			m_isMinimized = false;
		}

		WindowManager::Get().GetMainWindow().Resize(e.GetWidth(), e.GetHeight());

		MainUpdate();

		return false;
	}

	bool Application::OnViewportResizeEvent(ViewportResizeEvent& e)
	{
		WindowManager::Get().GetMainWindow().SetViewportSize(e.GetWidth(), e.GetHeight());
		return false;
	}

	bool Application::OnKeyPressedEvent(KeyPressedEvent&)
	{
		return false;
	}
}
