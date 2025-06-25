#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Core/RHICommon.h"
#include "RHIModule/RHIModule.h"
#include "RHIModule/Graphics/GraphicsContext.h"

#include <SubSystem/SubSystem.h>
#include <EventSystem/EventListener.h>

namespace Volt
{
	class AppPreRenderEvent;
}

namespace Volt::RHI
{
	class GraphicsContext;

	typedef RHIModule* (*PFN_CreateRHIModule)();
	typedef void (*PFN_DestroyRHIModule)(RHIModule* rhiModule);

	inline static constexpr const char* RHI_CREATE_FUNC_NAME = "CreateRHIModule";
	inline static constexpr const char* RHI_DESTROY_FUNC_NAME = "DestroyRHIModule";

	class VTRHI_API RHIModuleLoader : public SubSystem, public EventListener
	{
	public:
		RHIModuleLoader();

		void Shutdown() override;
		void LoadRHI(RHI::GraphicsAPI api, const RHI::RHICallbackInfo& callbackInfo);

		VT_DECLARE_SUBSYSTEM("{3E52F9E9-B7E0-4FAC-B728-0BBEE5CDE831}"_guid);
	private:
		typedef void* RHIModuleHandle;

		void LoadRHIFromFilepath(const std::filesystem::path& filepath);
		void CreateGraphicsContextForRHI(RHI::GraphicsAPI api, const RHI::RHICallbackInfo& callbackInfo);
		bool OnPreRenderEvent(AppPreRenderEvent& event);

		RHIModule* m_rhiModule = nullptr;
		RefPtr<RHI::GraphicsContext> m_graphicsContext;

		RHIModuleHandle m_rhiModuleHandle = nullptr;
	};
}
