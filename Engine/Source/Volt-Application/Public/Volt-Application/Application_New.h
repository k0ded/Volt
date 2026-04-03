#pragma once

#include "Volt-Application/Config.h"
#include "Volt-Application/BaseApplication.h"
#include "Volt-Application/ApplicationLayerStack.h"

#include <EventSystem/EventListener.h>

#include <CoreModule/MultiTimer.h>

#include <SubSystem/SubSystemManager.h>

#include <CoreUtilities/Pointers/Unique.h>

namespace Volt
{
	class Application_New;

	class ApplicationEventListener : public EventListener
	{
	public:
		ApplicationEventListener(Application_New& application);

	private:
		Application_New& m_application;
	};

	class VTAPP_API Application_New : public BaseApplication
	{
	public:
		Application_New(const CommandLineBuilder& commandLineBuilder, const ApplicationCreationInfo& createInfo = {});
		~Application_New() override;

		void Run() override;
		void Quit() override;
		void Tick() override;
		uint64_t GetFrameIndex() const override;

		void PushLayer(ApplicationLayer* layer) override;
		void PopLayer(ApplicationLayer* layer) override;

	protected:
		void LaunchMainWindow() override;

	private:
		friend class ApplicationEventListener;

		void RenderApplication();
		void EngineLoop();
		void OnAnyWindowRepaint();

		Unique<SubSystemManager> m_subSystemManager;
		Unique<ApplicationEventListener> m_eventListener;

		ApplicationLayerStack m_layerStack;
		MultiTimer m_frameTimer;

		class WindowManager_New* m_windowManager = nullptr;
		class ApplicationRenderer* m_renderer = nullptr;

		uint64_t m_frameIndex = 0;
		float m_currentDeltaTime = 0.f;

		bool m_isRunning = true;
	};
}
