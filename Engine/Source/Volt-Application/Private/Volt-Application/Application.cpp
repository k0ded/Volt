#include "vtapppch.h"
#include "Volt-Application/Application.h"
#include "Volt-Application/UI/ImGuiSubSystem.h"

#include <WindowModule/Events/WindowEvents.h>
#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

#include <InputModule/Events/KeyboardEvents.h>
#include <SubSystem/SubSystemManager.h>
#include <LogModule/Log.h>	

#include <EventSystem/ApplicationEvents.h>
#include <EventSystem/EventSystem.h>

#include <Volt-Core/PluginSystem/PluginSystem.h>
#include <Volt-Core/PluginSystem/PluginRegistry.h>
#include <Volt-Core/Project/ProjectManager.h>

#include <Volt-Renderer/Renderer.h>

#include <AssetSystem/AssetSerializerRegistry.h>
#include <AssetSystem/AssetFactory.h>

#include <CoreUtilities/FileSystem.h>

#include <RHIModule/FrameCapture.h>
#include <RHIModule/RHIModuleLoader.h>


namespace Volt
{
	ApplicationEventListener::ApplicationEventListener(Application& application)
		: m_application(application)
	{
		RegisterListener<AppUpdateEvent>(VT_BIND_EVENT_FN(ApplicationEventListener::OnAppUpdateEvent));
		RegisterListener<WindowCloseEvent>(VT_BIND_EVENT_FN(ApplicationEventListener::OnWindowCloseEvent));
		RegisterListener<WindowResizeEvent>(VT_BIND_EVENT_FN(ApplicationEventListener::OnWindowResizeEvent));
		RegisterListener<ViewportResizeEvent>(VT_BIND_EVENT_FN(ApplicationEventListener::OnViewportResizeEvent));
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

	Application::Application(const CommandLineBuilder& commandLineBuilder, const ApplicationCreationInfo& createInfo)
		: BaseApplication(commandLineBuilder, createInfo)
	{
		FileSystem::Initialize();
		FileSystem::InitializeWorkingDirectory(IsRuntime(), commandLineBuilder);

		m_subSystemManager = CreateScope<SubSystemManager>();
		m_subSystemManager->InitializeSubSystems(SubSystemInitializationStage::PreEngine);

		m_rhiModuleLoader = SubSystemManager::GetSubSystem<RHI::RHIModuleLoader>();
		m_logSubSystem = SubSystemManager::GetSubSystem<Log>();
		m_logSubSystem->EnableLogging(IsLoggingEnabled());

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

		m_assetManager = CreateScope<AssetManager>(ProjectManager::GetRootDirectory(), ProjectManager::GetAssetsDirectory(), ProjectManager::GetEngineRootDirectory());
		m_sourceAssetManager = CreateScope<SourceAssetManager>();

		m_windowManager = SubSystemManager::GetSubSystem<WindowManager>();

		if (m_appCreateInfo.createMainWindow)
		{
			LaunchMainWindow();
		}

		m_subSystemManager->InitializeSubSystems(SubSystemInitializationStage::Engine);

		//TODO: this is a hack because we dont have access to the AssetManager in all application types
		Renderer* rendererSubsystem = SubSystemManager::GetSubSystem<Renderer>();
		rendererSubsystem->CreateBlueNoise();

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

		m_subSystemManager->InitializeSubSystems(SubSystemInitializationStage::PostEngine);

		m_imguiSubSystem = SubSystemManager::GetSubSystem<ImGuiSubSystem>();
		// Make sure that the main window exits, it is required to initialize ImGui.
		if (m_appCreateInfo.createMainWindow && m_appCreateInfo.enableImGui)
		{
			m_imguiSubSystem->InitializeImGui(m_appCreateInfo.enableImGuiViewports);
		}

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

		//Amp::WWiseEngine::Get().TermWwise();

		m_assetManager->Clear();

		m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::Engine);

		m_assetManager = nullptr;

		m_windowManager->DestroyMainWindow();

		WindowManager::ShutdownGLFW();

		m_pluginSystem->UnloadPlugins();
		m_pluginSystem = nullptr;
		m_pluginRegistry = nullptr;
		m_projectManager = nullptr;

		m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::PreEngine);

		FileSystem::Shutdown();

		m_subSystemManager = nullptr;
	}

	void Application::Run()
	{
		VT_PROFILE_THREAD("Main");

		m_isRunning = true;

		while (m_isRunning)
		{
			VT_PROFILE_FRAME("Frame");
			MainUpdate();
		}
	}

	void Application::Quit()
	{
		m_isRunning = false;
	}

	void Application::PushLayer(ApplicationLayer* layer)
	{
		m_layerStack.PushLayer(layer);
	}

	void Volt::Application::PopLayer(ApplicationLayer* layer)
	{
		m_layerStack.PopLayer(layer);
	}

	void Application::LaunchMainWindow()
	{
		if (!m_windowManager->HasMainWindow())
		{
			WindowProperties windowProperties{};
			windowProperties.Width = m_appCreateInfo.width;
			windowProperties.Height = m_appCreateInfo.height;
			windowProperties.VSync = m_appCreateInfo.useVSync;
			windowProperties.Title = m_appCreateInfo.title;
			windowProperties.WindowMode = m_appCreateInfo.windowMode;
			windowProperties.IconPath = m_appCreateInfo.iconPath;
			windowProperties.CursorPath = m_appCreateInfo.cursorPath;
			windowProperties.UseTitlebar = m_appCreateInfo.useTitlebar;
			windowProperties.UseCustomTitlebar = m_appCreateInfo.useCustomTitlebar;

			if (m_appCreateInfo.isRuntime)
			{
				windowProperties.Title = ProjectManager::GetProject().name;
				windowProperties.CursorPath = ProjectManager::GetProject().cursorFilepath;
				windowProperties.IconPath = ProjectManager::GetProject().iconFilepath;
			}

			m_windowManager->CreateMainWindow(windowProperties);

			if (m_imguiSubSystem && m_appCreateInfo.enableImGui)
			{
				// Make sure that the main window exits, it is required to initialize ImGui.
				m_imguiSubSystem->InitializeImGui(m_appCreateInfo.enableImGuiViewports);
			}

			//if we are already running, we have to skip a frame so that we dont start trying to render witout beginning rendering
			if (m_isRunning)
			{
				m_skipPresentThisFrame = true;
			}
		}
	}

	void Application::MainUpdate()
	{
		m_isProcessingFrame = true;
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

		if (m_appCreateInfo.enableImGui && m_imguiSubSystem->IsInitialized() && !m_skipPresentThisFrame)
		{
			VT_PROFILE_SCOPE("Application::ImGui");

			m_imguiSubSystem->Begin();

			AppImGuiUpdateEvent imguiEvent{};
			EventSystem::DispatchEvent(imguiEvent);
		}

		{
			VT_PROFILE_SCOPE("Application::PostFrameUpdate");
			AppPostFrameUpdateEvent postFrameUpdateEvent{ m_currentDeltaTime };
			EventSystem::DispatchEvent(postFrameUpdateEvent);
		}

		if (m_appCreateInfo.enableImGui && m_imguiSubSystem->IsInitialized() && !m_skipPresentThisFrame)
		{
			m_imguiSubSystem->End();
		}

		m_isProcessingFrame = false;

		if (!m_skipPresentThisFrame)
		{
			WindowManager::Get().Present();
		}
		m_skipPresentThisFrame = false;

		m_frameTimer.Accumulate();
	}

	void Application::CreateGraphicsContext()
	{
		RHI::RHICallbackInfo callbackInfo{};
		callbackInfo.requestCloseEventCallback = []()
		{
			WindowCloseEvent closeEvent{};
			EventSystem::DispatchEvent(closeEvent);
		};

		m_rhiModuleLoader->LoadRHI(RHI::GraphicsAPI::Vulkan, callbackInfo);
	}

	void Application::SetupFrameCapture()
	{
		if (RHI::RHIModule::GetInstance().GetFrameCapture())
		{
			RHI::RHIModule::GetInstance().GetFrameCapture()->SetFlags(RHI::FrameCaptureFlags::DisableOverlay);
			RHI::RHIModule::GetInstance().GetFrameCapture()->SetCaptureFileTargetFilePath(ProjectManager::GetProjectDirectory() / ("Volt-" + ProjectManager::GetProject().name));
		}
	}


	bool Application::OnAppUpdateEvent(class AppUpdateEvent& e)
	{
		return false;
	}

	bool Application::OnWindowCloseEvent(class WindowCloseEvent& e)
	{
		m_isRunning = false;
		return false;
	}

	bool Application::OnWindowResizeEvent(class WindowResizeEvent& e)
	{
		WindowManager::Get().GetMainWindow().Resize(e.GetWidth(), e.GetHeight());

		if (!m_isProcessingFrame)
		{
			MainUpdate();
		}
		return false;
	}

	bool Application::OnViewportResizeEvent(class ViewportResizeEvent& e)
	{
		WindowManager::Get().GetMainWindow().SetViewportSize(e.GetWidth(), e.GetHeight());
		return false;
	}
}
