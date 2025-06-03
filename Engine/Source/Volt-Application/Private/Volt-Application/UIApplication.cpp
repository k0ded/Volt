#include "vtapppch.h"

#include "Volt-Application/UIApplication.h"
#include "Volt-Application/UI/ImGuiSubSystem.h"

#include <Volt-Renderer/Renderer.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>
#include <WindowModule/Events/WindowEvents.h>

#include <RHIModule/Graphics/GraphicsContext.h>
#include <VulkanRHIModule/VulkanRHIProxy.h>
#include <D3D12RHIModule/D3D12RHIProxy.h>
#include <RenderCore/RenderGraph/RenderGraphExecutionThread.h>

#include <CoreUtilities/Allocator.h>
#include <CoreUtilities/Allocators/PagedHeapAllocator.h>
#include <CoreUtilities/FileSystem.h>

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
		g_heapAllocator = CreateScope<PagedHeapAllocator>();

		FileSystem::Initialize();
		FileSystem::InitializeWorkingDirectory(createInfo.isRuntime, commandLineBuilder);

		m_subSystemManager = CreateScope<SubSystemManager>(SubSystemInclusionLevel::Minimal);
		m_subSystemManager->InitializeSubSystems(SubSystemInitializationStage::PreEngine);

		// This is required because glfwInit must be called before setting up graphics device
		WindowManager::InitializeGLFW();
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
			m_imguiSubSystem->SetupContext();
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

		m_graphicsContext = nullptr;
		m_rhiProxy = nullptr;
		WindowManager::ShutdownGLFW();

		m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::PreEngine);

		FileSystem::Shutdown();

		m_subSystemManager = nullptr;

		g_heapAllocator.reset();
	}

	void UIApplication::Run()
	{
		VT_PROFILE_THREAD("Main");

		m_isRunning = true;

		while (m_isRunning)
		{
			VT_PROFILE_FRAME("Frame");
			MainUpdate();

			//m_frameIndex++;
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
			windowProperties.Width = m_appCreateInfo.width;
			windowProperties.Height = m_appCreateInfo.height;
			windowProperties.VSync = m_appCreateInfo.useVSync;
			windowProperties.Title = m_appCreateInfo.title;
			windowProperties.WindowMode = m_appCreateInfo.windowMode;
			windowProperties.IconPath = m_appCreateInfo.iconPath;
			windowProperties.CursorPath = m_appCreateInfo.cursorPath;
			windowProperties.UseTitlebar = m_appCreateInfo.useTitlebar;
			windowProperties.UseCustomTitlebar = m_appCreateInfo.useCustomTitlebar;

			m_windowManager->CreateMainWindow(windowProperties);

			if (m_imguiSubSystem)
			{
				m_imguiSubSystem->InitializeImGui(m_appCreateInfo.enableImGuiViewports);
				m_imguiSubSystem->SetupContext();
			}
		}
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
			callbackInfo.requestCloseEventCallback = []()
			{
				WindowCloseEvent closeEvent{};
				EventSystem::DispatchEvent(closeEvent);
			};

			m_rhiProxy->SetRHICallbackInfo(callbackInfo);
		}

		m_graphicsContext = RHI::GraphicsContext::Create(cinfo);
	}

	void UIApplication::MainUpdate()
	{
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
		}

		if (m_info.enableImGui && m_imguiSubSystem->IsInitialized() /*&& !m_skipPresentThisFrame*/)
		{
			VT_PROFILE_SCOPE("Application::ImGui");

			m_imguiSubSystem->Begin();

			AppImGuiUpdateEvent imguiEvent{};
			EventSystem::DispatchEvent(imguiEvent);

			// #TODO_Ivar: HACK! Will keep this here for now. We need to make sure that the scene renderer output image is ready. 
			RenderGraphExecutionThread::WaitForFinishedExecution();
			m_imguiSubSystem->End();
		}
		else
		{
			RenderGraphExecutionThread::WaitForFinishedExecution();
		}

		{
			VT_PROFILE_SCOPE("Application::PostFrameUpdate");
			AppPostFrameUpdateEvent postFrameUpdateEvent{ m_currentDeltaTime };
			EventSystem::DispatchEvent(postFrameUpdateEvent);
		}

		//if (!m_skipPresentThisFrame)
		{
			WindowManager::Get().Present();
		}
		//m_skipPresentThisFrame = false;

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

		MainUpdate();
		return false;
	}

	bool UIApplication::OnViewportResizeEvent(class ViewportResizeEvent& e)
	{
		WindowManager::Get().GetMainWindow().SetViewportSize(e.GetWidth(), e.GetHeight());
		return false;
	}
	
}
