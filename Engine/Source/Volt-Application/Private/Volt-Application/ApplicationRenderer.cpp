#include "vtapppch.h"

#include "Volt-Application/ApplicationRenderer.h"

#include <Volt-Scene/SceneManager.h>
#include <Volt-Renderer/RenderScene.h>
#include <Volt-Renderer/SceneRenderer.h>

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RHIModule/RHIModuleLoader.h>

#include <SubSystem/SubSystemManager.h>

#include <JobSystem/TaskGraph.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(ApplicationRenderer, Minimal, Engine);

	void ApplicationRenderer::Initialize()
	{
		m_samplerStateCache = CreateUnique<SamplerStateCache>();
		m_commandBufferPool = CreateUnique<CommandBufferPool>();
		m_transientResourceAllocator = CreateUnique<TransientResourceAllocator>();

		m_sceneManager = SubSystemManager::GetSubSystem<SceneManager>();
		m_rhiModuleLoader = SubSystemManager::GetSubSystem<RHI::RHIModuleLoader>();
	}
	
	void ApplicationRenderer::Shutdown()
	{
		m_sceneManager = nullptr;

		m_transientResourceAllocator.Reset();
		m_commandBufferPool.Reset();
		m_samplerStateCache.Reset();
	}

	void ApplicationRenderer::Render(float timestep, uint64_t frameIndex)
	{
		VT_PROFILE_FUNCTION();

		PreRender(frameIndex);

		const ArrayView<SceneContainer*> sceneContainers = m_sceneManager->GetSceneContainers();

		// Update render scenes
		{
			VT_PROFILE_SCOPE("Update Render Scenes");

			//TaskGraph renderSceneUpdateGraph{ ExecutionPriority::Render };

			for (SceneContainer* container : sceneContainers)
			{
				Ref<RenderScene> renderScene = container->GetRenderScene();
				if (renderScene)
				{
					//renderSceneUpdateGraph.AddTask("Update RenderScene", [this, renderScene, frameIndex]() 
					{
						UpdateRenderScene(*renderScene, frameIndex);
					}
					//, FiberStackSize::KB64);
				}
			}

			//renderSceneUpdateGraph.ExecuteAndWait();
		}

		// Render scene renderers
		{
			//TaskGraph sceneRendererGraph{ ExecutionPriority::Render };

			for (SceneContainer* container : sceneContainers)
			{
				for (const Ref<SceneRenderer>& sceneRenderer : container->GetSceneRenderers())
				{
					//sceneRendererGraph.AddTask("Render SceneRenderer", [this, sceneRenderer, timestep]()
					{
						RenderSceneRenderer(*sceneRenderer, timestep);
					}
					//, FiberStackSize::KB64);
				}
			}

			//sceneRendererGraph.ExecuteAndWait();
		}

		PostRender();
	}

	void ApplicationRenderer::GetSubSystemDependencies(SubSystemDependencyList& outDependencies)
	{
		outDependencies.AddDependency<SceneManager>();
		outDependencies.AddDependency<RHI::RHIModuleLoader>();
	}

	void ApplicationRenderer::UpdateRenderScene(RenderScene& renderScene, uint64_t frameIndex)
	{
		RenderGraph renderGraph{};
		renderScene.BeginFrame(frameIndex);
		renderScene.Update(renderGraph);
		renderScene.EndFrame(renderGraph);

		renderGraph.Compile();
		renderGraph.Execute();
	}

	void ApplicationRenderer::RenderSceneRenderer(SceneRenderer& sceneRenderer, float timestep)
	{
		sceneRenderer.OnRender(timestep);
	}

	void ApplicationRenderer::PreRender(uint64_t frameIndex)
	{
		VT_PROFILE_FUNCTION();
	
		m_rhiModuleLoader->OnPreRender();
		m_transientResourceAllocator->OnPreRender(frameIndex);
		m_commandBufferPool->Update();
	}

	void ApplicationRenderer::PostRender()
	{
		m_rhiModuleLoader->OnPostRender();
	}
}
