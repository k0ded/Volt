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

#include <Volt-Core/Project/ProjectManager.h>

#include <Volt-Renderer/Renderer.h>

#include <AssetSystem/AssetFactory.h>
#include <AssetSystem/AssetManager.h>

#include <RHIModule/FrameCapture.h>
#include <RHIModule/RHIModuleLoader.h>

#include <CoreUtilities/FileSystem.h>
#include <CoreUtilities/Profiling/Profiling.h>

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

		m_subSystemManager = CreateUnique<SubSystemManager>();
		m_subSystemManager->InitializeSubSystems(SubSystemInitializationStage::PreEngine);

		m_rhiModuleLoader = SubSystemManager::GetSubSystem<RHI::RHIModuleLoader>();
		m_logSubSystem = SubSystemManager::GetSubSystem<Log>();
		m_logSubSystem->EnableLogging(IsLoggingEnabled());

		CreateGraphicsContext(commandLineBuilder);

		m_sourceAssetManager = CreateUnique<SourceAssetManager>();
		// #TODO_AssetSystem: Move to a sub system.
		g_assetManager = CreateUnique<AssetManager>(ProjectManager::GetEngineRootDirectory(), ProjectManager::GetRootDirectory(), ProjectManager::GetAssetsDirectoryName());

		m_windowManager = SubSystemManager::GetSubSystem<WindowManager>();
		{
			m_windowManager->SetForceSDR(m_commandLineBuilder.IsArgDefined("forcesdr"));
		}

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

		m_navigationSystem = CreateUnique<Volt::AI::NavigationSystem>();

		m_subSystemManager->InitializeSubSystems(SubSystemInitializationStage::PostEngine);
		m_subSystemManager->OnPostInitialization();

		m_imguiSubSystem = SubSystemManager::GetSubSystem<ImGuiSubSystem>();
		// Make sure that the main window exits, it is required to initialize ImGui.
		if (m_appCreateInfo.createMainWindow && m_appCreateInfo.enableImGui)
		{
			m_imguiSubSystem->InitializeImGui(m_appCreateInfo.enableImGuiViewports);
		}

		m_scriptingSystem = CreateUnique<ScriptingSystem>();
		m_eventListener = CreateUnique<ApplicationEventListener>(*this);

		SetupFrameCapture();
	}

	Application::~Application()
	{
		m_eventListener = nullptr;
		m_scriptingSystem = nullptr;

		m_subSystemManager->OnPreShutdown();
		m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::PostEngine);

		m_navigationSystem = nullptr;
		m_layerStack.Clear();

		//Amp::WWiseEngine::Get().TermWwise();

		m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::Engine);

		m_sourceAssetManager = nullptr;
		g_assetManager = nullptr;

		m_windowManager->DestroyMainWindow();

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
			VT_PROFILE_FRAME_START("Frame");

			MainUpdate();

			VT_PROFILE_FRAME_END("Frame");
		}
	}

	void Application::Quit()
	{
		m_isRunning = false;
	}

	void Application::Tick()
	{
		VT_PROFILE_FUNCTION();

		m_currentDeltaTime = m_frameTimer.GetDeltaTime();
		m_frameTimer.Update();

		m_frameIndex++;

		EventSystem::Update();

		AppTickEvent tickEvent(m_currentDeltaTime, m_frameIndex);
		EventSystem::DispatchEvent(tickEvent);
	}

	uint64_t Application::GetFrameIndex() const
	{
		return m_frameIndex;
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
			windowProperties.width = m_appCreateInfo.width;
			windowProperties.height = m_appCreateInfo.height;
			windowProperties.vsync = m_appCreateInfo.useVSync;
			windowProperties.title = m_appCreateInfo.title;
			windowProperties.windowMode = m_appCreateInfo.windowMode;
			windowProperties.iconPath = m_appCreateInfo.iconPath;
			windowProperties.cursorPath = m_appCreateInfo.cursorPath;
			windowProperties.useTitlebar = m_appCreateInfo.useTitlebar;
			windowProperties.useCustomTitlebar = m_appCreateInfo.useCustomTitlebar;
			//windowProperties.createAsDecorated = m_appCreateInfo.useTitlebar && !m_appCreateInfo.useCustomTitlebar;

			if (m_appCreateInfo.isRuntime)
			{
				windowProperties.title = ProjectManager::GetProject().name;
				windowProperties.cursorPath = ProjectManager::GetProject().cursorFilepath;
				windowProperties.iconPath = ProjectManager::GetProject().iconFilepath;
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

		AppBeginFrameEvent appBeginFrameEvent{};
		EventSystem::DispatchEvent(appBeginFrameEvent);

		Tick();

		{
			VT_PROFILE_SCOPE("Application::Render");

			AppPreRenderEvent preRenderEvent(m_frameIndex);
			EventSystem::DispatchEvent(preRenderEvent);

			AppRenderEvent renderEvent(m_currentDeltaTime);
			EventSystem::DispatchEvent(renderEvent);

			m_windowManager->Render(m_currentDeltaTime);
		}

		{
			VT_PROFILE_SCOPE("Application::Update");

			AppUpdateEvent updateEvent(m_currentDeltaTime);
			EventSystem::DispatchEvent(updateEvent);
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

		AppPresentFrameEvent appPresentEvent{};
		EventSystem::DispatchEvent(appPresentEvent);

		m_isProcessingFrame = false;
		if (!m_skipPresentThisFrame)
		{
			WindowManager::Get().Present();
		}
		m_skipPresentThisFrame = false;

		m_frameTimer.Accumulate();
	}

	void Application::CreateGraphicsContext(const CommandLineBuilder& commandLineBuilder)
	{
		RHI::RHICallbackInfo callbackInfo{};
		callbackInfo.requestCloseEventCallback = []()
		{
			WindowCloseEvent closeEvent{ WindowManager::Get().GetMainWindow() };
			EventSystem::DispatchEvent(closeEvent);
		};

		RHI::RHIConfig rhiConfig;
		rhiConfig.api = RHI::GraphicsAPI::Vulkan;
		rhiConfig.enableDebugLayer = false;
		rhiConfig.pipelineCacheFilepath = ProjectManager::GetProjectDirectory() / "Generated" / "PipelineCache.bin";

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

		m_rhiModuleLoader->LoadRHI(rhiConfig, callbackInfo);
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
		/*if (&e.GetWindow() == &WindowManager::Get().GetMainWindow())
		{
			WindowManager::Get().GetMainWindow().Resize(e.GetWidth(), e.GetHeight());

			if (!m_isProcessingFrame)
			{
				MainUpdate();
			}
		}*/

		return false;
	}

	bool Application::OnViewportResizeEvent(class ViewportResizeEvent& e)
	{
		WindowManager::Get().GetMainWindow().SetViewportSize(e.GetWidth(), e.GetHeight());
		return false;
	}
}
