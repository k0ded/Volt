#include "Testing/RenderGraphTests/DrawTriangleTest.h"

#include <RenderCore/RenderGraph2/RenderGraph2.h>
#include <RenderCore/RenderGraph2/RenderContext2.h>
#include <RenderCore/RenderGraph/RenderContextUtils.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/GlobalShader.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

using namespace Volt;

struct DrawTriangleTestVS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(DrawTriangleTestVS)

	BEGIN_SHADER_PARAMETER_STRUCT2(Parameters)
	END_SHADER_PARAMETER_STRUCT2()
};
REGISTER_SHADER_2(DrawTriangleTestVS, "Engine/Shaders/Source/Testing/RenderGraph/RG_DrawTriangleTest.hlsl", "MainVS", Vertex);

struct DrawTriangleTestPS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(DrawTriangleTestPS)

	BEGIN_SHADER_PARAMETER_STRUCT2(Parameters)
		RG_RENDER_TARGETS()
	END_SHADER_PARAMETER_STRUCT2()
};
REGISTER_SHADER_2(DrawTriangleTestPS, "Engine/Shaders/Source/Testing/RenderGraph/RG_DrawTriangleTest.hlsl", "MainPS", Pixel);

static RefPtr<RHI::Shader2> s_vertexShader;
static RefPtr<RHI::Shader2> s_pixelShader;
static RefPtr<RHI::RenderPipeline> s_renderPipeline;

RG_DrawTriangleTest::RG_DrawTriangleTest()
{
	s_vertexShader = ShaderMap::Get2<DrawTriangleTestVS>();
	s_pixelShader = ShaderMap::Get2<DrawTriangleTestPS>();

	RHI::RenderPipelineCreateInfo pipelineInfo{};
	pipelineInfo.shaders = { s_vertexShader, s_pixelShader };

	s_renderPipeline = RHI::RenderPipeline::Create2(pipelineInfo);
}

RG_DrawTriangleTest::~RG_DrawTriangleTest()
{
}

bool RG_DrawTriangleTest::RunTest()
{
	auto& swapchain = Volt::WindowManager::Get().GetMainWindow().GetSwapchain();

	RenderGraph2 renderGraph{ m_commandBuffer };

	auto targetImage = swapchain.GetCurrentImage();
	RGTextureRef swapchainTexture = renderGraph.RegisterExternalTexture(targetImage);

	DrawTriangleTestPS::Parameters* passParameters = renderGraph.AllocParameters<DrawTriangleTestPS::Parameters>();
	passParameters->renderTargets.renderTargets[0] = swapchainTexture;

	renderGraph.AddPass("Triangle Pass",
		RenderGraphPassFlags::NeverCull,
		passParameters,
		[passParameters, targetImage](RenderContext2& context) 
		{
			RenderingInfo2 renderingInfo = context.CreateRenderingInfo(targetImage->GetWidth(), targetImage->GetHeight(), passParameters->renderTargets);

			context.BeginRendering(renderingInfo);
			context.BindPipeline(s_renderPipeline);
			context.SetParameters<DrawTriangleTestPS>(s_pixelShader, passParameters);
			context.Draw(3, 0, 0, 0);
			context.EndRendering();
		});

	renderGraph.Compile();
	renderGraph.Execute();

	//
	//RenderGraph renderGraph{ m_commandBuffer };
	//
	//auto targetImage = swapchain.GetCurrentImage();
	//RenderGraphImageHandle targetImageHandle = renderGraph.AddExternalImage(targetImage);
	//
	//renderGraph.AddPass("Triangle Pass", 
	//[&](RenderGraph::Builder& builder) 
	//{
	//	builder.WriteResource(targetImageHandle);
	//	builder.SetHasSideEffect();
	//}, 
	//[=](RenderContext& context)
	//{
	//	RenderingInfo renderingInfo = context.CreateRenderingInfo(targetImage->GetWidth(), targetImage->GetHeight(), { targetImageHandle });
	//
	//	RHI::RenderPipelineCreateInfo pipelineInfo{};
	//	pipelineInfo.shaders = { ShaderMap::Get2<DrawTriangleTestVS>(), ShaderMap::Get2<DrawTriangleTestPS>() };
	//
	//	auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);
	//
	//	context.BeginRendering(renderingInfo);
	//	RCUtils::DrawFullscreenTriangle(context, pipeline);
	//	context.EndRendering();
	//});
	//
	//renderGraph.Compile();
	//renderGraph.Execute();

	return true;
}
