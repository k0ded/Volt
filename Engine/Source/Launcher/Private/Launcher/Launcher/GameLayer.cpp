#include "Launcher/GameLayer.h"

#include <Volt-Scene/Scene.h>
#include <Volt-Scene/SceneEvents.h>

#include <Volt-Renderer/SceneRenderer.h>

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/CopyToSwapchainShaders.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/DefaultShaders.h>

#include <CoreModule/Project/ProjectManager.h>

#include <Navigation/Core/NavigationSystem.h>

#include <WindowModule/Events/WindowEvents_New.h>
#include <WindowModule/WindowManager_New.h>
#include <WindowModule/Window_New.h>
#include <EventSystem/EventSystem.h>

using namespace Volt;

GameLayer::GameLayer(Volt::WindowHandle window)
	: m_window(window)
{

}

void GameLayer::OnAttach()
{
	RegisterListener<Volt::AppUpdateEvent>(VT_BIND_EVENT_FN(GameLayer::OnUpdateEvent));
	RegisterListener<Volt::AppRenderEvent>(VT_BIND_EVENT_FN(GameLayer::OnRenderEvent));
	RegisterListener<Volt::WindowResizeEvent_New>(VT_BIND_EVENT_FN(GameLayer::OnWindowResizeEvent));
	RegisterListener<Volt::WindowRenderEvent_New>(VT_BIND_EVENT_FN(GameLayer::OnWindowRenderEvent));

	Window_New& window = WindowManager_New::Get().GetWindow(m_window);

	m_scene = Scene::CreateDefaultScene("Test");
	
	SceneRendererCreateInfo createInfo{};
	createInfo.renderScene = m_scene->GetRenderScene();
	createInfo.drawDebug = true;
	createInfo.initialResolution = { window.GetWidth(), window.GetHeight() };
	
	m_sceneRenderer = CreateRef<SceneRenderer>(createInfo);
	
	m_camera = CreateRef<Camera>(glm::radians(60.f), 16.f / 9.f, 0.1f, 100000.f);
	m_camera->SetPosition({ 0.f, 0.f, -200.f });
}

void GameLayer::OnDetach()
{
}

bool GameLayer::OnUpdateEvent(Volt::AppUpdateEvent& e)
{
	return false;
}

bool GameLayer::OnRenderEvent(Volt::AppRenderEvent& e)
{
	m_sceneRenderer->OnRenderEditor(m_camera, e.GetTimestep());

	return false;
}

bool GameLayer::OnWindowResizeEvent(Volt::WindowResizeEvent_New& e)
{
	m_sceneRenderer->Resize(e.GetWidth(), e.GetHeight());
	return false;
}

bool GameLayer::OnWindowRenderEvent(Volt::WindowRenderEvent_New& e)
{
	if (e.GetWindow().GetHandle() != m_window)
	{
		return false;
	}

	const RHI::Swapchain& swapchain = e.GetWindow().GetSwapchain();

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
	renderGraph.Execute();

	return true;
}
