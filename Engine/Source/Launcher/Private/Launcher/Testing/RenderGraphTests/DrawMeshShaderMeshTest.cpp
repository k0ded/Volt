#include "Testing/RenderGraphTests/DrawMeshShaderMeshTest.h"

#include <Volt-Assets/MeshAsset.h>

#include <Volt-Renderer/Mesh/Mesh.h>
#include <Volt-Renderer/Renderer.h>

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <AssetSystem/AssetManager.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

using namespace Volt;

RG_DrawMeshShaderMeshTest::RG_DrawMeshShaderMeshTest()
	: m_commandBufferSet(Renderer::GetFramesInFlight())
{
	m_mesh = AssetManager::GetAsset<MeshAsset>("Engine/Meshes/Primitives/SM_Cube.vtasset")->GetMesh();
}

RG_DrawMeshShaderMeshTest::~RG_DrawMeshShaderMeshTest()
{
}

bool RG_DrawMeshShaderMeshTest::RunTest()
{
	auto& swapchain = Volt::WindowManager::Get().GetMainWindow().GetSwapchain();

	RenderGraph renderGraph{ m_commandBufferSet.IncrementAndGetCommandBuffer() };

	auto targetImage = swapchain.GetCurrentImage();
	RenderGraphImageHandle targetImageHandle = renderGraph.AddExternalImage(targetImage);

	struct Data
	{
		RenderGraphBufferHandle vertexPositionBuffer;
		RenderGraphBufferHandle meshletsBuffer;
		RenderGraphBufferHandle meshletDataBuffer;
	};

	//const auto& gpuMesh = m_mesh->GetGPUMeshes().at(0);

	//renderGraph.AddPass<Data>("Mesh Pass",
	//[&](RenderGraph::Builder& builder, Data& data)
	//{
	//	data.vertexPositionBuffer = builder.AddExternalBuffer(m_mesh->GetVertexPositionsBuffer()->GetResource());
	//	data.meshletsBuffer = builder.AddExternalBuffer(m_mesh->GetMeshletBuffer()->GetResource());
	//	data.meshletDataBuffer = builder.AddExternalBuffer(m_mesh->GetMeshletDataBuffer()->GetResource());

	//	builder.WriteResource(targetImageHandle);

	//	builder.ReadResource(data.vertexPositionBuffer);
	//	builder.ReadResource(data.meshletsBuffer);
	//	builder.ReadResource(data.meshletDataBuffer);

	//	builder.SetHasSideEffect();
	//},
	//[=](const Data& data, RenderContext& context)
	//{
	//	RenderingInfo renderingInfo = context.CreateRenderingInfo(targetImage->GetWidth(), targetImage->GetHeight(), { targetImageHandle });

	//	RHI::RenderPipelineCreateInfo pipelineInfo{};
	//	pipelineInfo.shader = ShaderMap::Get("RG_DrawMeshShaderMeshTest");
	//	pipelineInfo.cullMode = RHI::CullMode::None;

	//	auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

	//	glm::mat4 viewProj = glm::perspective(glm::radians(60.f), 16.f / 9.f, 1000.f, 0.1f) * glm::lookAt({ 0.f, 200.f, -500.f }, { 0.f }, glm::vec3(0.f, 1, 0.f));

	//	context.BeginRendering(renderingInfo);
	//	context.BindPipeline(pipeline);

	//	context.SetConstant("ViewProjection"_sh, viewProj);

	//	context.SetConstant("VertexPositionsBuffer"_sh, data.vertexPositionBuffer);
	//	context.SetConstant("MeshletsBuffer"_sh, data.meshletsBuffer);
	//	context.SetConstant("MeshletDataBuffer"_sh, data.meshletDataBuffer);

	//	context.SetConstant("MeshletStartOffset"_sh, gpuMesh.meshletStartOffset);
	//	context.SetConstant("VertexOffset"_sh, gpuMesh.vertexStartOffset);

	//	context.DispatchMeshTasks(gpuMesh.meshletCount, 1, 1);
	//	context.EndRendering();
	//});

	renderGraph.Compile();
	renderGraph.Execute();

	return true;
}
