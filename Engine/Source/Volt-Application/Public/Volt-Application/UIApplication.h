#pragma once

#include "Volt-Application/Config.h"
#include "Volt-Application/BaseApplication.h"
#include "Volt-Application/ApplicationLayerStack.h"

#include <Volt-Core/MultiTimer.h>

#include <SubSystem/SubSystemManager.h>

#include <CoreUtilities/Pointers/RefPtr.h>

namespace Volt
{
	class ImGuiSubSystem;
	class WindowManager;

	namespace RHI
	{
		class ImGuiImplementation;
		class GraphicsContext;
		class RHIProxy;
	}

	// A application type that should be used for UI only applictions,
	// does not provide game systems such as physics, ...
	class VTAPP_API UIApplication : public BaseApplication
	{
	public:
		UIApplication(const CommandLineBuilder& commandLineBuilder, const ApplicationCreationInfo& createInfo = {});
		~UIApplication() override;

		void Run() override;
		void Quit() override;

		void PushLayer(ApplicationLayer* layer) override;
		void PopLayer(ApplicationLayer* layer) override;

	protected:
		void LaunchMainWindow() override;
	private:
		void CreateGraphicsContext();
		void MainUpdate();

		const ApplicationCreationInfo m_info;

		ApplicationLayerStack m_layerStack;
		MultiTimer m_frameTimer;

		RefPtr<RHI::GraphicsContext> m_graphicsContext;
		RefPtr<RHI::RHIProxy> m_rhiProxy;

		Scope<SubSystemManager> m_subSystemManager;

		WindowManager* m_windowManager = nullptr;
		ImGuiSubSystem* m_imguiSubSystem = nullptr;

		bool m_isRunning = false;
		float m_currentDeltaTime = 0.f;
	};
}
