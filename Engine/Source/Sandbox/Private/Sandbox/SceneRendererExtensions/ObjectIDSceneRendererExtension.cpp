#include "sbpch.h"

#include "Sandbox/SceneRendererExtensions/ObjectIDSceneRendererExtension.h"
#include "Sandbox/SceneRendererExtensions/ObjectIDPassMeshProcessor.h"

#include <Volt-Renderer/SceneRendererStructs.h>
#include <Volt-Renderer/RendererCommon.h>
#include <Volt-Renderer/GPUScene.h>
#include <Volt-Renderer/RenderView.h>
#include <Volt-Renderer/RenderScene.h>
#include <Volt-Renderer/SceneRendererRenderGraphData.h>

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/ShaderParameterStruct.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/BatchedShaderParameters.h>

using namespace Volt;

VT_REGISTER_SHADER(ObjectIDVS, "Engine/Shaders/Source/Editor/ObjectID.hlsl", "MainVS", Vertex);
VT_REGISTER_SHADER(ObjectIDPS, "Engine/Shaders/Source/Editor/ObjectID.hlsl", "MainPS", Pixel);

BEGIN_SHADER_PARAMETER_STRUCT(ObjectIDParameters)
	SHADER_PARAMETER_STRUCT_INCLUDE(ObjectIDVS::Parameters, VS)
	SHADER_PARAMETER_STRUCT_INCLUDE(ObjectIDPS::Parameters, PS)
END_SHADER_PARAMETER_STRUCT()

Volt::RGTextureRef ObjectIDSceneRendererExtension::OnRender(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, const Volt::RenderView& view, Volt::RGTextureRef prevOutputImage)
{
	const SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();

	RHI::RenderPipelineCreateInfo pipelineInfo;
	pipelineInfo.depthMode = RHI::DepthMode::Read;

	RGTextureRef objectIdTexture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R32_UINT>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "ObjectID"));

	ObjectIDTexture& objectIdTextureStruct = blackboard.Add<ObjectIDTexture>();
	objectIdTextureStruct.texture = objectIdTexture;

	ObjectIDParameters* passParameters = renderGraph.AllocParameters<ObjectIDParameters>();
	passParameters->VS.View = view.viewUniformBuffer;
	passParameters->VS.GPUScene = m_renderScene->GetGPUSceneParameters(renderGraph);
	passParameters->PS.renderTargets.renderTargets[0] = objectIdTexture;
	passParameters->PS.renderTargets.depthTarget = sceneTextures.sceneDepth;

	m_meshPassProcessor->PrepareRenderCommands(renderGraph);

	renderGraph.AddPass("Render Object ID",
		RenderGraphPassFlags::None,
		passParameters, 
		[passParameters, view, meshPassProcessor = m_meshPassProcessor](RenderContext& context)
	{
		BatchedShaderParameters batchedShaderParameters;
		context.CollectParameters(passParameters, batchedShaderParameters);

		RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->PS.renderTargets);
		renderingInfo.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;

		context.BeginRendering(renderingInfo);
		meshPassProcessor->ExecuteCommands(context, batchedShaderParameters);
		context.EndRendering();
	});

	renderGraph.EnqueueTextureExtraction(objectIdTexture, &m_objectIdImage);

	return prevOutputImage;
}

void ObjectIDSceneRendererExtension::OnRegistered(Volt::MeshPassProcessorRegistry& meshPassProcessorRegistry)
{
	m_meshPassProcessor = meshPassProcessorRegistry.AddProcessor<ObjectIDPassMeshProcessor>();
}
