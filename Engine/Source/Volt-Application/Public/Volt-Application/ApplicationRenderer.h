#pragma once

#include <RenderCore/TransientResourceSystem/TransientResourceAllocator.h>
#include <RenderCore/CommandBufferPool.h>
#include <RenderCore/SamplerStateCache.h>

#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <CoreUtilities/Pointers/Unique.h>

namespace Volt
{
	namespace RHI
	{
		class RHIModuleLoader;
	}

	class RenderScene;
	class SceneRenderer;

	class ApplicationRenderer : public SubSystem
	{
	public:
		~ApplicationRenderer() override = default;

		void Initialize() override;
		void Shutdown() override;

		void Render(float timestep, uint64_t frameIndex);

		static void GetSubSystemDependencies(SubSystemDependencyList& outDependencies);
		VT_DECLARE_SUBSYSTEM("{6596CB16-B988-421B-BC68-7F01DCA6D758}"_guid);

	private:
		void UpdateRenderScene(RenderScene& renderScene, uint64_t frameIndex);
		void RenderSceneRenderer(SceneRenderer& sceneRenderer, float timestep);

		void PreRender(uint64_t frameIndex);
		void PostRender();

		class SceneManager* m_sceneManager = nullptr;
		RHI::RHIModuleLoader* m_rhiModuleLoader = nullptr;

		Unique<SamplerStateCache> m_samplerStateCache;
		Unique<CommandBufferPool> m_commandBufferPool;
		Unique<TransientResourceAllocator> m_transientResourceAllocator;
	};
}
