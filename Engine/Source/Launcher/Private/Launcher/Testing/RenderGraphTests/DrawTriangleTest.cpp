#include "Testing/RenderGraphTests/DrawTriangleTest.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderContext.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/GlobalShader.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

using namespace Volt;

struct DrawTriangleTestVS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(DrawTriangleTestVS)

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
	END_SHADER_PARAMETER_STRUCT()
};
REGISTER_SHADER_2(DrawTriangleTestVS, "Engine/Shaders/Source/Testing/RenderGraph/RG_DrawTriangleTest.hlsl", "MainVS", Vertex);

struct DrawTriangleTestPS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(DrawTriangleTestPS)

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		RG_RENDER_TARGETS()
	END_SHADER_PARAMETER_STRUCT()
};
REGISTER_SHADER_2(DrawTriangleTestPS, "Engine/Shaders/Source/Testing/RenderGraph/RG_DrawTriangleTest.hlsl", "MainPS", Pixel);

static RefPtr<RHI::Shader> s_vertexShader;
static RefPtr<RHI::Shader> s_pixelShader;
static RefPtr<RHI::RenderPipeline> s_renderPipeline;

RG_DrawTriangleTest::RG_DrawTriangleTest()
{
	s_vertexShader = ShaderMap::Get2<DrawTriangleTestVS>();
	s_pixelShader = ShaderMap::Get2<DrawTriangleTestPS>();

	RHI::RenderPipelineCreateInfo pipelineInfo{};
	pipelineInfo.shaders = { s_vertexShader, s_pixelShader };

	s_renderPipeline = RHI::RenderPipeline::Create(pipelineInfo);
}

RG_DrawTriangleTest::~RG_DrawTriangleTest()
{
}

bool RG_DrawTriangleTest::RunTest()
{
	auto& swapchain = Volt::WindowManager::Get().GetMainWindow().GetSwapchain();

	RenderGraph renderGraph{ m_commandBuffer };

	auto targetImage = swapchain.GetCurrentImage();
	RGTextureRef swapchainTexture = renderGraph.RegisterExternalTexture(targetImage);

	DrawTriangleTestPS::Parameters* passParameters = renderGraph.AllocParameters<DrawTriangleTestPS::Parameters>();
	passParameters->renderTargets.renderTargets[0] = swapchainTexture;

	renderGraph.AddPass("Triangle Pass",
		RenderGraphPassFlags::NeverCull,
		passParameters,
		[passParameters, targetImage](RenderContext& context) 
		{
			RenderingInfo2 renderingInfo = context.CreateRenderingInfo(targetImage->GetWidth(), targetImage->GetHeight(), passParameters->renderTargets);

			context.BeginRendering(renderingInfo);
			context.BindPipeline(s_renderPipeline);
			context.SetParameters<DrawTriangleTestPS>(s_pixelShader, passParameters);
			context.Draw(3, 1, 0, 0);
			context.EndRendering();
		});

	renderGraph.Compile();
	renderGraph.Execute();

	return true;
}
