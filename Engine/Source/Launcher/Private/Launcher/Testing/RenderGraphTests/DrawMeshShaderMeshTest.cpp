#include "Testing/RenderGraphTests/DrawMeshShaderMeshTest.h"

#include <Volt-Assets/MeshAsset.h>

#include <Volt-Renderer/Mesh/Mesh.h>
#include <Volt-Renderer/Renderer.h>

#include <RenderCore/RenderGraph2/RenderGraph2.h>
#include <RenderCore/RenderGraph2/RenderContext2.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/GlobalShader.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>

#include <AssetSystem/AssetManager.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

using namespace Volt;

struct DrawMeshTestMS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(DrawMeshTestMS)

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<float3>, VertexPositionsBuffer)
		SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<Meshlet>, MeshletsBuffer)
		SHADER_PARAMETER_BUFFER_SRV(Buffer<uint>, MeshletDataBuffer)
		SHADER_PARAMETER(uint, MeshletStartOffset)
		SHADER_PARAMETER(uint, VertexOffset)
		SHADER_PARAMETER(float4x4, ViewProjection)
	END_SHADER_PARAMETER_STRUCT()
};
REGISTER_SHADER_2(DrawMeshTestMS, "Engine/Shaders/Source/Testing/RenderGraph/RG_DrawMeshShaderMeshTest.hlsl", "MainMS", Mesh);

struct DrawMeshTestPS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(DrawTriangleTest2PS)

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		RG_RENDER_TARGETS()
	END_SHADER_PARAMETER_STRUCT()
};
REGISTER_SHADER_2(DrawMeshTestPS, "Engine/Shaders/Source/Testing/RenderGraph/RG_DrawMeshShaderMeshTest.hlsl", "MainPS", Pixel);

BEGIN_SHADER_PARAMETER_STRUCT(DrawMeshParameters)
	SHADER_PARAMETER_STRUCT_INCLUDE(DrawMeshTestMS::Parameters, MS)
	SHADER_PARAMETER_STRUCT_INCLUDE(DrawMeshTestPS::Parameters, PS)
END_SHADER_PARAMETER_STRUCT()

static RefPtr<RHI::Shader2> s_meshShader;
static RefPtr<RHI::Shader2> s_pixelShader;
static RefPtr<RHI::RenderPipeline> s_renderPipeline;

RG_DrawMeshShaderMeshTest::RG_DrawMeshShaderMeshTest()
	: m_commandBufferSet(Renderer::GetFramesInFlight())
{
	s_meshShader = ShaderMap::Get2<DrawMeshTestMS>();
	s_pixelShader = ShaderMap::Get2<DrawMeshTestPS>();

	RHI::RenderPipelineCreateInfo pipelineInfo{};
	pipelineInfo.shaders = { s_meshShader, s_pixelShader };

	s_renderPipeline = RHI::RenderPipeline::Create2(pipelineInfo);

	m_mesh = AssetManager::GetAsset<MeshAsset>("Engine/Meshes/Primitives/SM_Cube.vtasset")->GetMesh();
}

RG_DrawMeshShaderMeshTest::~RG_DrawMeshShaderMeshTest()
{
}

bool RG_DrawMeshShaderMeshTest::RunTest()
{
	auto& swapchain = Volt::WindowManager::Get().GetMainWindow().GetSwapchain();
	const auto& gpuMesh = m_mesh->GetGPUMeshes().at(0);
	const glm::mat4 viewProj = glm::perspective(glm::radians(60.f), 16.f / 9.f, 1000.f, 0.1f) * glm::lookAt({ 0.f, 200.f, -500.f }, { 0.f }, glm::vec3(0.f, 1, 0.f));

	RenderGraph2 renderGraph{ m_commandBuffer };

	auto targetImage = swapchain.GetCurrentImage();
	RGTextureRef swapchainTexture = renderGraph.RegisterExternalTexture(targetImage);

	RGBufferRef vertexPositionsBuffer = renderGraph.RegisterExternalBuffer(m_mesh->GetVertexPositionsBuffer()->GetResource());
	RGBufferRef meshletsBuffer = renderGraph.RegisterExternalBuffer(m_mesh->GetMeshletBuffer()->GetResource());
	RGBufferRef meshletDataBuffer = renderGraph.RegisterExternalBuffer(m_mesh->GetMeshletDataBuffer()->GetResource());

	DrawMeshParameters* passParameters = renderGraph.AllocParameters<DrawMeshParameters>();
	passParameters->MS.VertexPositionsBuffer = renderGraph.CreateSRV(vertexPositionsBuffer);
	passParameters->MS.MeshletsBuffer = renderGraph.CreateSRV(meshletsBuffer);
	passParameters->MS.MeshletDataBuffer = renderGraph.CreateSRV(meshletDataBuffer);
	passParameters->MS.MeshletStartOffset = gpuMesh.meshletStartOffset;
	passParameters->MS.VertexOffset = gpuMesh.vertexStartOffset;
	passParameters->MS.ViewProjection = viewProj;
	passParameters->PS.renderTargets.renderTargets[0] = swapchainTexture;

	renderGraph.AddPass("Draw Mesh Pass",
		RenderGraphPassFlags::None,
		passParameters,
		[passParameters, targetImage, gpuMesh](RenderContext2& context)
	{
		RenderingInfo2 renderingInfo = context.CreateRenderingInfo(targetImage->GetWidth(), targetImage->GetHeight(), passParameters->PS.renderTargets);

		context.BeginRendering(renderingInfo);
		context.BindPipeline(s_renderPipeline);
		context.SetParameters<DrawMeshTestMS>(s_meshShader, &passParameters->MS);
		context.SetParameters<DrawMeshTestPS>(s_pixelShader, &passParameters->PS);
		context.DispatchMeshTasks(gpuMesh.meshletCount, 1, 1);
		context.EndRendering();
	});

	renderGraph.Compile();
	renderGraph.Execute();

	return true;
}
