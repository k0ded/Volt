#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Core/RHICommon.h"
#include "RHIModule/RHIModule.h"
#include "RHIModule/Graphics/GraphicsContext.h"

#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

namespace Volt::RHI
{
	class GraphicsContext;

	typedef RHIModule* (*PFN_CreateRHIModule)();
	typedef void (*PFN_DestroyRHIModule)(RHIModule* rhiModule);

	inline static constexpr const char* RHI_CREATE_FUNC_NAME = "CreateRHIModule";
	inline static constexpr const char* RHI_DESTROY_FUNC_NAME = "DestroyRHIModule";

	struct RHIConfig
	{
		RHI::GraphicsAPI api;
		bool enableDebugLayer = false;

		Filesystem::Path pipelineCacheFilepath;
	};

	class VTRHI_API RHIModuleLoader : public SubSystem
	{
	public:
		RHIModuleLoader();

		void Initialize() override;
		void Shutdown() override;
		void LoadRHI(const RHIConfig& rhiConfig, const RHI::RHICallbackInfo& callbackInfo);

		void OnPreRender();
		void OnPostRender();

		static void GetSubSystemDependencies(SubSystemDependencyList& outDependencies);
		VT_DECLARE_SUBSYSTEM("{3E52F9E9-B7E0-4FAC-B728-0BBEE5CDE831}"_guid);
	private:
		typedef void* RHIModuleHandle;

		void LoadRHIFromFilepath(const Filesystem::Path& filepath);
		void CreateGraphicsContextForRHI(const RHIConfig& rhiConfig, const RHI::RHICallbackInfo& callbackInfo);

		void SetupRHIConfiguration();

		RHIModule* m_rhiModule = nullptr;
		IntRef<RHI::GraphicsContext> m_graphicsContext;

		RHIModuleHandle m_rhiModuleHandle = nullptr;
	};
}
