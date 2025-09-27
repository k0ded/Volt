#include "Launcher/TestingLayer.h"

#include "Volt-Scene/Scene.h"

#include "Volt-Renderer/SceneRenderer.h"
#include "Volt-Renderer/Camera/Camera.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/ShaderRegistry.h>
#include <RenderCore/Shader/PipelineStateCache.h>
#include <RenderCore/Shader/DefaultShaders.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/RenderGraph/RenderContext.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

#include <imgui.h>

using namespace Volt;

struct WriteFullscreenColorPS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(WriteFullscreenColorPS)
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER(float4, Color)
		RG_RENDER_TARGETS()
	END_SHADER_PARAMETER_STRUCT()
};
REGISTER_SHADER(WriteFullscreenColorPS, "Engine/Shaders/Source/Tests/WriteFullscreenColor.hlsl", "MainPS", Pixel);

BEGIN_SHADER_PARAMETER_STRUCT(WriteFullscreenColorParameters)
	SHADER_PARAMETER_STRUCT_INCLUDE(FullscreenTriangleVS::Parameters, VS)
	SHADER_PARAMETER_STRUCT_INCLUDE(WriteFullscreenColorPS::Parameters, PS)
END_SHADER_PARAMETER_STRUCT()

void TestingLayer::OnAttach()
{
	RegisterListener<Volt::AppUpdateEvent>(VT_BIND_EVENT_FN(TestingLayer::OnUpdateEvent));
	RegisterListener<Volt::AppRenderEvent>(VT_BIND_EVENT_FN(TestingLayer::OnRenderEvent));
	RegisterListener<Volt::WindowResizeEvent>(VT_BIND_EVENT_FN(TestingLayer::OnWindowResizeEvent));
	RegisterListener<Volt::AppImGuiUpdateEvent>(VT_BIND_EVENT_FN(TestingLayer::OnImGuiUpdateEvent));

	m_scene = Scene::CreateDefaultScene("Test");
	
	SceneRendererCreateInfo createInfo{};
	createInfo.renderScene = m_scene->GetRenderScene();
	createInfo.drawDebug = true;
	
	m_sceneRenderer = CreateRef<SceneRenderer>(createInfo);

	m_camera = CreateRef<Camera>(glm::radians(60.f), 16.f / 9.f, 0.1f, 100000.f);
}

void TestingLayer::OnDetach()
{

}

bool TestingLayer::OnUpdateEvent(Volt::AppUpdateEvent& e)
{
	return false;
}

bool TestingLayer::OnRenderEvent(Volt::AppRenderEvent& e)
{
#if 0
	RefPtr<RHI::Image> currentSwapchainImage = WindowManager::Get().GetMainWindow().GetSwapchain().GetCurrentImage();

	const uint32_t width = currentSwapchainImage->GetWidth();
	const uint32_t height = currentSwapchainImage->GetHeight();

	RenderGraph renderGraph{};

	for (uint32_t i = 0; i < 100; ++i)
	{
		WriteFullscreenColorParameters* passParameters = renderGraph.AllocParameters<WriteFullscreenColorParameters>();
		passParameters->PS.Color = glm::vec4(1.f, 0.f, 0.f, 1.f);
		passParameters->PS.renderTargets.renderTargets[0] = renderGraph.RegisterExternalTexture(currentSwapchainImage);

		auto vertexShader = ShaderMap::Get<FullscreenTriangleVS>();
		auto pixelShader = ShaderMap::Get<WriteFullscreenColorPS>();

		renderGraph.AddPass("Clear",
			RenderGraphPassFlags::NeverCull,
			passParameters,
			[passParameters, vertexShader, pixelShader, width, height](RenderContext& context)
		{
			RHI::RenderPipelineCreateInfo pipelineInfo{};
			pipelineInfo.shaders = { vertexShader, pixelShader };
			pipelineInfo.cullMode = RHI::CullMode::None;
			pipelineInfo.depthMode = RHI::DepthMode::None;

			auto pipeline = PipelineStateCache::GetRenderPipeline(pipelineInfo);

			RenderingInfo renderingInfo = context.CreateRenderingInfo(width, height, passParameters->PS.renderTargets);

			context.BeginRendering(renderingInfo);
			context.BindPipeline(pipeline);
			context.SetParameters<WriteFullscreenColorPS>(pixelShader, &passParameters->PS);
			context.Draw(3, 1, 0, 0);
			context.EndRendering();
		});
	}

	renderGraph.Compile();
	renderGraph.Execute();
#else
	m_sceneRenderer->OnRenderEditor(m_camera, e.GetTimestep());
#endif

	return false;
}

bool TestingLayer::OnImGuiUpdateEvent(Volt::AppImGuiUpdateEvent& e)
{
	ImGui::ShowDemoWindow();
	return false;
}

bool TestingLayer::OnWindowResizeEvent(Volt::WindowResizeEvent& e)
{
	m_sceneRenderer->Resize(e.GetWidth(), e.GetHeight());
	return false;
}
