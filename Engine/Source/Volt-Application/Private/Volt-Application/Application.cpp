#include "vtapppch.h"
#include "Volt-Application/Application.h"

#include <WindowModule/Events/WindowEvents.h>
#include <InputModule/Events/KeyboardEvents.h>
#include <EventSystem/ApplicationEvents.h>
#include <SubSystem/SubSystemManager.h>

#include <Volt-Core/PluginSystem/PluginSystem.h>

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

	Application::Application(const CommandLineBuilder& commandLineBuilder, const ApplicationCreationInfo& createInfo)
		: BaseApplication(commandLineBuilder, createInfo)
	{}

	Application::~Application()
	{
		//m_eventListener = nullptr;
		//m_pluginSystem->ShutdownPlugins();

		//m_scriptingSystem = nullptr;

		//m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::PostEngine);

		//m_navigationSystem = nullptr;
		//m_layerStack.Clear();
		//SceneManager::Shutdown();

		////Amp::WWiseEngine::Get().TermWwise();

		//m_assetManager->Clear();

		//m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::Engine);

		//m_assetManager = nullptr;
		//g_assetSerializerRegistry.Clear();
		//g_assetFactory.Clear();

		//m_windowManager->DestroyMainWindow();

		//m_graphicsContext = nullptr;
		//m_rhiProxy = nullptr;
		//WindowManager::ShutdownGLFW();

		//m_pluginSystem->UnloadPlugins();
		//m_pluginSystem = nullptr;
		//m_pluginRegistry = nullptr;
		//m_projectManager = nullptr;

		//m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::PreEngine);

		//FileSystem::Shutdown();

		//m_subSystemManager = nullptr;

		//g_heapAllocator.reset();
		//s_instance = nullptr;
	}

	void Application::PushLayer(ApplicationLayer* layer)
	{}

	void Volt::Application::PopLayer(ApplicationLayer* layer)
	{}
}
