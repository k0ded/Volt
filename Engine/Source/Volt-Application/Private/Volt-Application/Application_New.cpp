#include "vtapppch.h"

#include "Volt-Application/Application_New.h"
#include "Volt-Application/ApplicationRenderer.h"

#include <WindowModule/WindowManager_New.h>

#include <JobSystem/JobSystem.h>
#include <EventSystem/ApplicationEvents.h>
#include <EventSystem/EventSystem.h>

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	Application_New::Application_New(const CommandLineBuilder& commandLineBuilder, const ApplicationCreationInfo& createInfo)
		: BaseApplication(commandLineBuilder, createInfo)
	{
		m_subSystemManager = CreateUnique<SubSystemManager>();
		m_subSystemManager->InitializeSubSystems(SubSystemInitializationStage::PreEngine);
		m_subSystemManager->InitializeSubSystems(SubSystemInitializationStage::Engine);
		m_subSystemManager->InitializeSubSystems(SubSystemInitializationStage::PostEngine);
		m_subSystemManager->OnPostInitialization();
		
		// Get the window manager
		m_windowManager = m_subSystemManager->GetSubSystem<WindowManager_New>();
		m_windowManager->GetOnAnyWindowRepaint().AddRaw(this, &Application_New::OnAnyWindowRepaint);

		// Get the renderer
		m_renderer = m_subSystemManager->GetSubSystem<ApplicationRenderer>();
	}

	Application_New::~Application_New()
	{
		m_layerStack.Clear();

		m_renderer = nullptr;
		m_windowManager = nullptr;

		m_subSystemManager->OnPreShutdown();
		m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::PostEngine);
		m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::Engine);
		m_subSystemManager->ShutdownSubSystems(SubSystemInitializationStage::PreEngine);
	}
	
	void Application_New::Run()
	{
		VT_PROFILE_THREAD("Main");

		while (m_isRunning)
		{
			VT_PROFILE_FRAME_START("Frame");

			m_currentDeltaTime = m_frameTimer.GetDeltaTime();
			m_frameTimer.Update();

			EventSystem::Update();

			m_windowManager->ProcessMessages();

			EngineLoop();

			m_windowManager->BeginFrame();
			m_windowManager->Render(m_currentDeltaTime);
			m_windowManager->Present();

			m_frameIndex++;
			VT_PROFILE_FRAME_END("Frame");
		}
	}
	
	void Application_New::Quit()
	{
		m_isRunning = false;
	}
	
	void Application_New::Tick()
	{}
	
	uint64_t Application_New::GetFrameIndex() const
	{
		return m_frameIndex;
	}
	
	void Application_New::PushLayer(ApplicationLayer* layer)
	{
		m_layerStack.PushLayer(layer);
	}
	
	void Application_New::PopLayer(ApplicationLayer * layer)
	{
		m_layerStack.PopLayer(layer);
	}
	
	void Application_New::LaunchMainWindow()
	{}

	void Application_New::RenderApplication()
	{
		if (m_renderer)
		{
			VT_PROFILE_SCOPE("Application::Render");
			m_renderer->Render(m_currentDeltaTime, m_frameIndex);
		}
	}

	void Application_New::EngineLoop()
	{
		{
			AppTickEvent tickEvent(m_currentDeltaTime, m_frameIndex);
			EventSystem::DispatchEvent(tickEvent);
		}

		{
			VT_PROFILE_SCOPE("Application::Update");

			AppUpdateEvent updateEvent(m_currentDeltaTime);
			EventSystem::DispatchEvent(updateEvent);
		}

		RenderApplication();

		{
			VT_PROFILE_SCOPE("Application::PostFrame");

			AppPostFrameUpdateEvent postFrameEvent(m_currentDeltaTime);
			EventSystem::DispatchEvent(postFrameEvent);
		}
	}

	void Application_New::OnAnyWindowRepaint()
	{
		EngineLoop();
	}
}
