#include "vtapppch.h"

#include "Volt-Application/UIApplication.h"
#include "Volt-Application/UI/ImGuiSubSystem.h"

#include <Volt-Renderer/Renderer.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>
#include <WindowModule/Events/WindowEvents.h>

#include <RHIModule/RHIModuleLoader.h>

#include <CoreUtilities/FileSystem.h>
#include <CoreUtilities/Profiling/Profiling.h>

#include <EventSystem/EventSystem.h>
#include <EventSystem/ApplicationEvents.h>

namespace Volt
{
	UIApplicationEventListener::UIApplicationEventListener(UIApplication& application)
		: m_application(application)
	{
		RegisterListener<AppUpdateEvent>(VT_BIND_EVENT_FN(UIApplicationEventListener::OnAppUpdateEvent));
		RegisterListener<WindowCloseEvent>(VT_BIND_EVENT_FN(UIApplicationEventListener::OnWindowCloseEvent));
		RegisterListener<WindowResizeEvent>(VT_BIND_EVENT_FN(UIApplicationEventListener::OnWindowResizeEvent));
		RegisterListener<ViewportResizeEvent>(VT_BIND_EVENT_FN(UIApplicationEventListener::OnViewportResizeEvent));
	}

	bool UIApplicationEventListener::OnAppUpdateEvent(AppUpdateEvent& e)
	{
		return m_application.OnAppUpdateEvent(e);
	}

	bool UIApplicationEventListener::OnWindowCloseEvent(WindowCloseEvent& e)
	{
		return m_application.OnWindowCloseEvent(e);
	}

	bool UIApplicationEventListener::OnWindowResizeEvent(WindowResizeEvent& e)
	{
		return m_application.OnWindowResizeEvent(e);
	}

	bool UIApplicationEventListener::OnViewportResizeEvent(ViewportResizeEvent& e)
	{
		return m_application.OnViewportResizeEvent(e);
	}

	UIApplication::UIApplication(const CommandLineBuilder& commandLineBuilder, const ApplicationCreationInfo& createInfo)
		: BaseApplication(commandLineBuilder, createInfo)
	{
		FileSystem::Initialize();
		FileSystem::InitializeWorkingDirectory(createInfo.isRuntime, commandLineBuilder);

		m_subSystemManager = CreateScope<SubSystemManager>(SubSystemInclusionLevel::Minimal);
		m_subSystemManager->InitializeSubSystems(SubSystemInitializationStage::PreEngine);

		m_rhiModuleLoader = SubSystemManager::GetSubSystem<RHI::RHIModuleLoader>();
		// This is required because glfwInit must be called before setting up graphics device
		CreateGraphicsContext();

		m_windowManager = SubSystemManager::GetSubSystem<WindowManager>();

		if (m_appCreateInfo.createMainWindow)
		{
			LaunchMainWindow();
		}

		m_subSystemManager->InitializeSubSystems(SubSystemInitializationStage::Engine);
		m_subSystemManager->InitializeSubSystems(SubSystemInitializationStage::PostEngine);

		m_imguiSubSystem = SubSystemManager::GetSubSystem<ImGuiSubSystem>();
		// Make sure that the main window exits, it is required to initialize ImGui.
		if (m_appCreateInfo.createMainWindow && m_appCreateInfo.enableImGui)
		{
			m_imguiSubSystem->InitializeImGui(m_appCreateInfo.enableImGuiViewports);
		}

		m_eventListener = CreateScope<UIApplicationEventListener>(*this);
	}

	UIApplication::~UIApplication()
	{
		m_eventListener = nullptr;

		m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::PostEngine);

		m_layerStack.Clear();

		m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::Engine);

		m_windowManager->DestroyMainWindow();

		m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::PreEngine);

		FileSystem::Shutdown();

		m_subSystemManager = nullptr;
	}

	void UIApplication::Run()
	{
		VT_PROFILE_THREAD("Main");

		m_isRunning = true;

		while (m_isRunning)
		{
			VT_PROFILE_FRAME("Frame");
			MainUpdate();
		}
	}

	void UIApplication::Quit()
	{
		m_isRunning = false;
	}

	void UIApplication::PushLayer(ApplicationLayer* layer)
	{
		m_layerStack.PushLayer(layer);
	}

	void UIApplication::PopLayer(ApplicationLayer* layer)
	{
		m_layerStack.PopLayer(layer);
	}

	void UIApplication::LaunchMainWindow()
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

			m_windowManager->CreateMainWindow(windowProperties);

			if (m_imguiSubSystem)
			{
				m_imguiSubSystem->InitializeImGui(m_appCreateInfo.enableImGuiViewports);
			}

			//if we are already running, we have to skip a frame so that we dont start trying to render witout beginning rendering
			if (m_isRunning)
			{
				m_skipPresentThisFrame = true;
			}
		}
	}

	void UIApplication::CreateGraphicsContext()
	{
		RHI::RHICallbackInfo callbackInfo{};
		callbackInfo.requestCloseEventCallback = []()
		{
			WindowCloseEvent closeEvent{ WindowManager::Get().GetMainWindow() };
			EventSystem::DispatchEvent(closeEvent);
		};

		m_rhiModuleLoader->LoadRHI(RHI::GraphicsAPI::Vulkan, callbackInfo);
	}

	void UIApplication::MainUpdate()
	{
		WindowManager::Get().BeginFrame();
		m_isProcessingFrame = true;

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
		}

		if (m_info.enableImGui && m_imguiSubSystem->IsInitialized() && !m_skipPresentThisFrame)
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

	bool UIApplication::OnAppUpdateEvent(class AppUpdateEvent& e)
	{
		return false;
	}

	bool UIApplication::OnWindowCloseEvent(class WindowCloseEvent& e)
	{
		m_isRunning = false;
		return false;
	}

	bool UIApplication::OnWindowResizeEvent(class WindowResizeEvent& e)
	{
		WindowManager::Get().GetMainWindow().Resize(e.GetWidth(), e.GetHeight());

		if (!m_isProcessingFrame)
		{
			MainUpdate();
		}
		return false;
	}

	bool UIApplication::OnViewportResizeEvent(class ViewportResizeEvent& e)
	{
		WindowManager::Get().GetMainWindow().SetViewportSize(e.GetWidth(), e.GetHeight());
		return false;
	}
	
}
