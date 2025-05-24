#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Core/RHICommon.h"
#include "RHIModule/RHIModule.h"

#include <SubSystem/SubSystem.h>

namespace Volt::RHI
{
	class GraphicsContext;

	typedef RHIModule* (*PFN_CreateRHIModule)();
	typedef void (*PFN_DestroyRHIModule)(RHIModule* rhiModule);

	inline static constexpr const char* RHI_CREATE_FUNC_NAME = "CreateRHIModule";
	inline static constexpr const char* RHI_DESTROY_FUNC_NAME = "DestroyRHIModule";

	class VTRHI_API RHIModuleLoader : public SubSystem
	{
	public:
		void LoadRHI(RHI::GraphicsAPI api, const RHI::RHICallbackInfo& callbackInfo);

		VT_DECLARE_SUBSYSTEM("{3E52F9E9-B7E0-4FAC-B728-0BBEE5CDE831}"_guid);
	private:
		typedef void* RHIModuleHandle;

		void LoadRHIFromFilepath(const std::filesystem::path& filepath);
		void CreateGraphicsContextForRHI(RHI::GraphicsAPI api, const RHI::RHICallbackInfo& callbackInfo);

		RHIModule* m_rhiModule = nullptr;
		RefPtr<RHI::GraphicsContext> m_graphicsContext;

		RHIModuleHandle m_rhiModuleHandle = nullptr;
	};
}
