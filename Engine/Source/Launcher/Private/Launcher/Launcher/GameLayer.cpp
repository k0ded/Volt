#include "Launcher/GameLayer.h"

#include <Volt-Scene/Scene.h>

#include <Volt-Application/BaseApplication.h>

#include <Volt-Renderer/SceneRenderer.h>

#include <Volt-Scene/SceneManager.h>

#include <SubSystem/SubSystemManager.h>

#include <JobSystem/JobSystem.h>

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/CopyToSwapchainShaders.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/DefaultShaders.h>

#include <WindowModule/WindowManager_New.h>
#include <WindowModule/Window_New.h>

using namespace Volt;

GameLayer::GameLayer(Volt::WindowHandle window)
	: m_window(window)
{

}

void GameLayer::OnAttach()
{
	RegisterListener<Volt::AppUpdateEvent>(VT_BIND_EVENT_FN(GameLayer::OnUpdateEvent));

	Window_New& window = WindowManager_New::Get().GetWindow(m_window);

	window.GetOnWindowClosed().AddLambda([windowHandle = m_window](Window_New&) 
	{
		WindowManager_New::Get().DestroyWindow(windowHandle);
		BaseApplication::Get().Quit();
	});

	window.GetOnWindowResize().AddLambda([this](Window_New&, uint32_t width, uint32_t height) 
	{
		if (m_sceneRenderer)
		{
			m_sceneRenderer->Resize(width, height);
		}
	});

	window.GetOnWindowRender().AddRaw(this, &GameLayer::RenderWindow);

	m_sceneManager = SubSystemManager::GetSubSystem<SceneManager>();
	m_sceneContainer = m_sceneManager->CreateMemoryScene("TestScene");

	SceneRendererInitializer createInfo{};
	createInfo.drawDebug = true;
	createInfo.initialResolution = { window.GetWidth(), window.GetHeight() };
	
	m_sceneRenderer = m_sceneContainer->AttachSceneRenderer(createInfo);

	m_camera = CreateRef<Camera>(glm::radians(60.f), 16.f / 9.f, 0.1f, 100000.f);
	m_camera->SetPosition({ 0.f, 0.f, -200.f });

	m_sceneRenderer->SetCamera(m_camera);
}

void GameLayer::OnDetach()
{
}

bool GameLayer::OnUpdateEvent(Volt::AppUpdateEvent& e)
{
	return false;
}

void GameLayer::RenderWindow(Volt::Window_New& window)
{
	const RHI::Swapchain& swapchain = window.GetSwapchain();

	RenderGraph renderGraph;

	CopyToSwapchain_SDR::Parameters* passParameters = renderGraph.AllocParameters<CopyToSwapchain_SDR::Parameters>();
	passParameters->SrcColor = renderGraph.CreateSRV(renderGraph.RegisterExternalTexture(m_sceneRenderer->GetFinalImage()));
	passParameters->renderTargets.renderTargets[0] = renderGraph.RegisterExternalTexture(swapchain.GetCurrentImage());

	auto vertexShader = ShaderMap::Get<FullscreenTriangleVS>();
	auto pixelShader = ShaderMap::Get<CopyToSwapchain_SDR>();

	const uint32_t width = swapchain.GetWidth();
	const uint32_t height = swapchain.GetHeight();

	renderGraph.AddPass("CopyToSwapchain",
		RenderGraphPassFlags::Raster,
		passParameters,
		[passParameters, vertexShader, pixelShader, width, height](RenderContext& context)
	{
		GraphicsPipelineState pipelineState{};
		pipelineState.shaders = { vertexShader, pixelShader };
		pipelineState.cullMode = RHI::CullMode::None;
		pipelineState.depthMode = RHI::DepthMode::None;
		pipelineState.renderTargets = passParameters->renderTargets;

		RenderingInfo renderingInfo = context.CreateRenderingInfo(width, height, passParameters->renderTargets);

		context.BeginRendering(renderingInfo);
		context.SetPipelineState(pipelineState);
		context.SetParameters<CopyToSwapchain_SDR>(pixelShader, passParameters);
		context.Draw(3, 1, 0, 0);
		context.EndRendering();
	});

	renderGraph.Compile();
	JobCounterRef counter = renderGraph.ExecuteAndExtractCounter();
	JobSystem::WaitForAndDestroyCounter(counter);
}
