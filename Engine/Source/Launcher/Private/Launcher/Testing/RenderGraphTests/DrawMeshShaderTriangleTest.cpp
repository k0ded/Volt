#include "Testing/RenderGraphTests/DrawMeshShaderTriangleTest.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderContext.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/GlobalShader.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

using namespace Volt;

struct DrawTriangleTestAS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(DrawTriangleTestAS)

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
	END_SHADER_PARAMETER_STRUCT()
};
REGISTER_SHADER_2(DrawTriangleTestAS, "Engine/Shaders/Source/Testing/RenderGraph/RG_DrawMeshShaderTriangleTest.hlsl", "AmpMain", Amplification);

struct DrawTriangleTestMS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(DrawTriangleTestMS)

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
	END_SHADER_PARAMETER_STRUCT()
};
REGISTER_SHADER_2(DrawTriangleTestMS, "Engine/Shaders/Source/Testing/RenderGraph/RG_DrawMeshShaderTriangleTest.hlsl", "MeshMain", Mesh);

struct DrawTriangleTest2PS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(DrawTriangleTest2PS)

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		RG_RENDER_TARGETS()
	END_SHADER_PARAMETER_STRUCT()
};
REGISTER_SHADER_2(DrawTriangleTest2PS, "Engine/Shaders/Source/Testing/RenderGraph/RG_DrawMeshShaderTriangleTest.hlsl", "MainPS", Pixel);

static RefPtr<RHI::Shader> s_amplificationShader;
static RefPtr<RHI::Shader> s_meshShader;
static RefPtr<RHI::Shader> s_pixelShader;
static RefPtr<RHI::RenderPipeline> s_renderPipeline;

RG_DrawMeshShaderTriangleTest::RG_DrawMeshShaderTriangleTest()
{
	s_amplificationShader = ShaderMap::Get2<DrawTriangleTestAS>();
	s_meshShader = ShaderMap::Get2<DrawTriangleTestMS>();
	s_pixelShader = ShaderMap::Get2<DrawTriangleTest2PS>();

	RHI::RenderPipelineCreateInfo pipelineInfo{};
	pipelineInfo.shaders = { s_amplificationShader, s_meshShader, s_pixelShader };
	pipelineInfo.cullMode = RHI::CullMode::None;

	s_renderPipeline = RHI::RenderPipeline::Create(pipelineInfo);
}

RG_DrawMeshShaderTriangleTest::~RG_DrawMeshShaderTriangleTest()
{
}

bool RG_DrawMeshShaderTriangleTest::RunTest()
{
	auto& swapchain = Volt::WindowManager::Get().GetMainWindow().GetSwapchain();

	RenderGraph renderGraph{ m_commandBuffer };

	auto targetImage = swapchain.GetCurrentImage();
	RGTextureRef swapchainTexture = renderGraph.RegisterExternalTexture(targetImage);

	DrawTriangleTest2PS::Parameters* passParameters = renderGraph.AllocParameters<DrawTriangleTest2PS::Parameters>();
	passParameters->renderTargets.renderTargets[0] = swapchainTexture;

	renderGraph.AddPass("Triangle Pass",
		RenderGraphPassFlags::NeverCull,
		passParameters,
		[passParameters, targetImage](RenderContext& context)
	{
		RenderingInfo2 renderingInfo = context.CreateRenderingInfo(targetImage->GetWidth(), targetImage->GetHeight(), passParameters->renderTargets);

		context.BeginRendering(renderingInfo);
		context.BindPipeline(s_renderPipeline);
		context.SetParameters<DrawTriangleTest2PS>(s_pixelShader, passParameters);
		context.DispatchMeshTasks(1, 1, 1);
		context.EndRendering();
	});

	renderGraph.Compile();
	renderGraph.Execute();

	return true;
}
