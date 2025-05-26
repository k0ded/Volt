#pragma once

#include "Volt-Application/BaseApplication.h"
#include "Volt-Application/Config.h"

#include <SubSystem/SubSystemManager.h>

#include <CoreUtilities/Pointers/RefPtr.h>

namespace Volt
{
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

	private:
		void CreateGraphicsContext();
		void MainUpdate();

		RefPtr<RHI::GraphicsContext> m_graphicsContext;
		RefPtr<RHI::RHIProxy> m_rhiProxy;

		Scope<SubSystemManager> m_subSystemManager;

		bool m_isRunning = false;
	};
}
