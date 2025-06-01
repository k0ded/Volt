#pragma once

#include "Volt-Application/Config.h"
#include "Volt-Application/BaseApplication.h"
#include "Volt-Application/ApplicationLayerStack.h"

#include <Volt-Core/MultiTimer.h>

#include <CoreUtilities/Pointers/RefPtr.h>

#include <EventSystem/EventListener.h>

#include <EntitySystem/Scripting/ScriptingSystem.h>
#include <SubSystem/SubSystemManager.h>
#include <AssetSystem/AssetManager.h>
#include <AssetSystem/SourceAssetManager.h>
#include <Navigation/Core/NavigationSystem.h>


namespace Volt
{
	class PluginRegistry;
	class PluginSystem;
	class DynamicLibraryManager;
	class EventSystem;
	class Input;
	class WindowManager;
	class PhysicsSubSystem;
	class ImGuiSubSystem;
	class ApplicationEventListener;
	class ProjectManager;
	class ProjectManager;
	class Log;

	namespace RHI
	{
		class ImGuiImplementation;
		class GraphicsContext;
		class RHIProxy;
	}


	class Application;

	class ApplicationEventListener : public EventListener
	{
	public:
		ApplicationEventListener(Application& application);

	private:
		bool OnAppUpdateEvent(class AppUpdateEvent& e);
		bool OnWindowCloseEvent(class WindowCloseEvent& e);
		bool OnWindowResizeEvent(class WindowResizeEvent& e);
		bool OnViewportResizeEvent(class ViewportResizeEvent& e);
		bool OnKeyPressedEvent(class KeyPressedEvent& e);

		Application& m_application;
	};

	class VTAPP_API Application : public BaseApplication
	{
	public:
		Application(const CommandLineBuilder& commandLineBuilder, const ApplicationCreationInfo& createInfo = {});
		~Application() override;

		void Run() override;
		void Quit() override;

		void PushLayer(ApplicationLayer* layer) override;
		void PopLayer(ApplicationLayer* layer) override;

		inline static Application& Get() { return reinterpret_cast<Application&>(Get()); }

		AI::NavigationSystem& GetNavigationSystem() { return *m_navigationSystem; }
	protected:
		void LaunchMainWindow() override;
	private:
		friend class ApplicationEventListener;

		void CreateGraphicsContext();
		void MainUpdate();

		bool OnAppUpdateEvent(class AppUpdateEvent& e);
		bool OnWindowCloseEvent(class WindowCloseEvent& e);
		bool OnWindowResizeEvent(class WindowResizeEvent& e);
		bool OnViewportResizeEvent(class ViewportResizeEvent& e);
		bool OnKeyPressedEvent(class KeyPressedEvent& e);

		ApplicationLayerStack m_layerStack;
		MultiTimer m_frameTimer;

		RefPtr<RHI::GraphicsContext> m_graphicsContext;
		RefPtr<RHI::RHIProxy> m_rhiProxy;

		Scope<SubSystemManager> m_subSystemManager;
		Scope<AssetManager> m_assetManager;
		Scope<SourceAssetManager> m_sourceAssetManager;
		Scope<ScriptingSystem> m_scriptingSystem;
		Scope<ApplicationEventListener> m_eventListener;
		Scope<AI::NavigationSystem> m_navigationSystem; //is this in use anywhere?

		ProjectManager* m_projectManager = nullptr;
		PluginRegistry* m_pluginRegistry = nullptr;
		PluginSystem* m_pluginSystem = nullptr;
		WindowManager* m_windowManager = nullptr;
		PhysicsSubSystem* m_physicsSubSystem = nullptr;
		ImGuiSubSystem* m_imguiSubSystem = nullptr;
		Log* m_logSubSystem = nullptr;

		bool m_isRunning = false;
		float m_currentDeltaTime = 0.f;
	};
}
