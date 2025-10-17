#pragma once

#include "Volt-Application/Config.h"
#include "Volt-Application/BaseApplication.h"
#include "Volt-Application/ApplicationLayerStack.h"

#include <Volt-Core/MultiTimer.h>

#include <SubSystem/SubSystemManager.h>

#include <EventSystem/EventListener.h>

#include <CoreUtilities/Pointers/RefPtr.h>

namespace Volt
{
	class ImGuiSubSystem;
	class WindowManager;

	namespace RHI
	{
		class ImGuiImplementation;
		class RHIModuleLoader;
	}

	class UIApplication;
	class VTAPP_API UIApplicationEventListener : public EventListener
	{
	public:
		UIApplicationEventListener(UIApplication& application);

	private:
		bool OnAppUpdateEvent(class AppUpdateEvent& e);
		bool OnWindowCloseEvent(class WindowCloseEvent& e);
		bool OnWindowResizeEvent(class WindowResizeEvent& e);
		bool OnViewportResizeEvent(class ViewportResizeEvent& e);

		UIApplication& m_application;
	};

	// A application type that should be used for UI only applictions,
	// does not provide game systems such as physics, ...
	class VTAPP_API UIApplication : public BaseApplication
	{
	public:
		UIApplication(const CommandLineBuilder& commandLineBuilder, const ApplicationCreationInfo& createInfo = {});
		~UIApplication() override;

		void Run() override;
		void Quit() override;
		void Tick() override;
		uint64_t GetFrameIndex() const override;

		void PushLayer(ApplicationLayer* layer) override;
		void PopLayer(ApplicationLayer* layer) override;

	protected:
		void LaunchMainWindow() override;
	private:
		friend class UIApplicationEventListener;

		void CreateGraphicsContext(const CommandLineBuilder& commandLineBuilder);
		void MainUpdate();

		bool OnAppUpdateEvent(class AppUpdateEvent& e);
		bool OnWindowCloseEvent(class WindowCloseEvent& e);
		bool OnWindowResizeEvent(class WindowResizeEvent& e);
		bool OnViewportResizeEvent(class ViewportResizeEvent& e);

		const ApplicationCreationInfo m_info;

		ApplicationLayerStack m_layerStack;
		MultiTimer m_frameTimer;

		Scope<SubSystemManager> m_subSystemManager;
		Scope<UIApplicationEventListener> m_eventListener;

		WindowManager* m_windowManager = nullptr;
		ImGuiSubSystem* m_imguiSubSystem = nullptr;
		RHI::RHIModuleLoader* m_rhiModuleLoader = nullptr;

		bool m_isRunning = false;
		bool m_skipPresentThisFrame = false;
		bool m_isProcessingFrame = false;
		float m_currentDeltaTime = 0.f;
		uint64_t m_frameIndex = 0;
	};
}
