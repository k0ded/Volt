#pragma once

#include "Volt-Application/Config.h"
#include "Volt-Application/BaseApplication.h"
#include "Volt-Application/ApplicationLayerStack.h"

#include <Volt-Core/MultiTimer.h>

#include <CoreUtilities/Pointers/RefPtr.h>

#include <EventSystem/EventListener.h>

#include <EntitySystem/Scripting/ScriptingSystem.h>
#include <SubSystem/SubSystemManager.h>
#include <AssetSystem/SourceAssetManager.h>
#include <Navigation/Core/NavigationSystem.h>

class Log;
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

	namespace RHI
	{
		class ImGuiImplementation;
		class RHIModuleLoader;
	}

	class Application;

	class VTAPP_API ApplicationEventListener : public EventListener
	{
	public:
		ApplicationEventListener(Application& application);

	private:
		bool OnAppUpdateEvent(class AppUpdateEvent& e);
		bool OnWindowCloseEvent(class WindowCloseEvent& e);
		bool OnWindowResizeEvent(class WindowResizeEvent& e);
		bool OnViewportResizeEvent(class ViewportResizeEvent& e);

		Application& m_application;
	};

	class VTAPP_API Application : public BaseApplication
	{
	public:
		Application(const CommandLineBuilder& commandLineBuilder, const ApplicationCreationInfo& createInfo = {});
		~Application() override;

		void Run() override;
		void Quit() override;
		void Tick() override;
		uint64_t GetFrameIndex() const override;

		void PushLayer(ApplicationLayer* layer) override;
		void PopLayer(ApplicationLayer* layer) override;

		inline static Application& Get() { return reinterpret_cast<Application&>(BaseApplication::Get()); }

		AI::NavigationSystem& GetNavigationSystem() { return *m_navigationSystem; }
		inline const float GetAverageFrameTime() const { return m_frameTimer.GetAverageTime(); }
		inline const float GetMaxFrameTime() const { return m_frameTimer.GetMaxFrameTime(); }
	protected:
		void LaunchMainWindow() override;
	private:
		friend class ApplicationEventListener;

		void MainUpdate();
		void CreateGraphicsContext(const CommandLineBuilder& commandLineBuilder);
		void SetupFrameCapture();

		bool OnAppUpdateEvent(class AppUpdateEvent& e);
		bool OnWindowCloseEvent(class WindowCloseEvent& e);
		bool OnWindowResizeEvent(class WindowResizeEvent& e);
		bool OnViewportResizeEvent(class ViewportResizeEvent& e);

		ApplicationLayerStack m_layerStack;
		MultiTimer m_frameTimer;

		Scope<SubSystemManager> m_subSystemManager;
		Scope<SourceAssetManager> m_sourceAssetManager;
		Scope<ScriptingSystem> m_scriptingSystem;
		Scope<ApplicationEventListener> m_eventListener;
		Scope<AI::NavigationSystem> m_navigationSystem; //is this in use anywhere?

		ProjectManager* m_projectManager = nullptr;
		PluginRegistry* m_pluginRegistry = nullptr;
		PluginSystem* m_pluginSystem = nullptr;
		WindowManager* m_windowManager = nullptr;
		ImGuiSubSystem* m_imguiSubSystem = nullptr;
		Log* m_logSubSystem = nullptr;
		RHI::RHIModuleLoader* m_rhiModuleLoader = nullptr;

		bool m_skipPresentThisFrame = false;
		bool m_isRunning = false;
		bool m_isProcessingFrame = false;
		float m_currentDeltaTime = 0.f;
		uint64_t m_frameIndex = 0;
	};
}
