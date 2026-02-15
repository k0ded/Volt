
#include "sbpch.h"

#include "Sandbox/SceneRendererExtensions/OutlineTechnique.h"
#include "Sandbox/SceneRendererExtensions/OutlineMeshPassProcessor.h"

#include <Volt-Renderer/GPUScene.h>
#include <Volt-Renderer/SceneRendererRenderGraphData.h>
#include <Volt-Renderer/RenderView.h>
#include <Volt-Renderer/RenderScene.h>

#include <Volt-Core/Algorithms.h>

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderContext.h>
#include <RenderCore/RenderGraph/ShaderRegistry.h>
#include <RenderCore/Shader/BatchedShaderParameters.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/DefaultShaders.h>
#include <RenderCore/Shader/PipelineStateCache.h>
#include <RenderCore/SamplerStateCache.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>

#include <CoreUtilities/Containers/AtomicBitVector.h>

using namespace Volt;

OutlineTechnique::OutlineTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, OutlineMeshPassProcessor* meshPassProcessor)
	: m_renderGraph(renderGraph), m_blackboard(blackboard), m_meshPassProcessor(meshPassProcessor)
{
}

void OutlineTechnique::Execute(RGTextureRef dstImage, RenderScene& renderScene, const RenderView& view, const std::unordered_set<Volt::EntityID>& selectedEntities)
{
	m_renderGraph.BeginMarker("Outline");

	RGTextureRef outlineGeometryTexture = AddDrawOutlineGeometryPass(renderScene, view, selectedEntities);
	RGTextureRef jumpFloodTexture = AddJumpFloodInitPass(outlineGeometryTexture, view);

	const int32_t numSteps = 2;
	int32_t step = (int32_t)std::round(std::pow(numSteps - 1, 2));

	while (step != 0)
	{
		jumpFloodTexture = AddJumpFloodPass(jumpFloodTexture, view, step);
		step /= 2;
	}

	AddOutlineCompositePass(dstImage, view, jumpFloodTexture);

	m_renderGraph.EndMarker();
}

VT_REGISTER_SHADER(OutlineGeometryVS, "Engine/Shaders/Source/Editor/Outline/OutlineGeometry.hlsl", "MainVS", Vertex);
VT_REGISTER_SHADER(OutlineGeometryPS, "Engine/Shaders/Source/Editor/Outline/OutlineGeometry.hlsl", "MainPS", Pixel);

BEGIN_SHADER_PARAMETER_STRUCT(OutlineGeometryParameters)
	SHADER_PARAMETER_STRUCT_INCLUDE(OutlineGeometryVS::Parameters, VS)
	SHADER_PARAMETER_STRUCT_INCLUDE(OutlineGeometryPS::Parameters, PS)
	SHADER_PARAMETER_STRUCT_INCLUDE(Volt::MeshPassProcessorParameters, ProcessorParameters)
END_SHADER_PARAMETER_STRUCT()

RGTextureRef OutlineTechnique::AddDrawOutlineGeometryPass(Volt::RenderScene& renderScene, const RenderView& view, const std::unordered_set<Volt::EntityID>& selectedEntities)
{
	Vector<RenderPrimitiveData*> renderPrimitives = renderScene.GetRenderPrimitives();
	AtomicBitVector<uint32_t> bitVector;
	bitVector.Resize(renderScene.GetMaxPrimitiveIndex() + 1);

	Algo::ForEachParalellBlocking([&](uint32_t threadIdx, uint32_t elementIdx)
	{
		RenderPrimitiveData* primitiveData = renderPrimitives[elementIdx];

		if (selectedEntities.contains(primitiveData->entityId))
		{
			bitVector.SetBit(renderScene.GetPrimitiveIndexFromID(primitiveData->id), true, std::memory_order::relaxed);
		}

	}, static_cast<uint32_t>(renderPrimitives.size()), 128);

	Vector<uint32_t> bitVectorCopy = bitVector.ToVector();

	RGBufferRef primitivesToDraw = m_renderGraph.CreateBuffer(RGBufferDesc::CreateMappableBufferDesc<uint32_t>(renderScene.GetMaxPrimitiveIndex() + 1, RHI::BufferUsage::StorageBuffer, "Outline.PrimitivesToDraw"));
	AddMappedBufferUploadCopyData(m_renderGraph, primitivesToDraw, bitVectorCopy.data(), bitVectorCopy.byte_size());

	RGTextureRef colorTexture = m_renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R8G8B8A8_UNORM>(view.width, view.height, RHI::ImageUsage::Attachment, "OutlineGeometryColor"));
	RGTextureRef depthTexture = m_renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::D32_SFLOAT>(view.width, view.height, RHI::ImageUsage::Attachment, "OutlineGeometryDepth"));

	m_meshPassProcessor->PrepareRenderCommands(m_renderGraph);

	OutlineGeometryParameters* passParameters = m_renderGraph.AllocParameters<OutlineGeometryParameters>();
	passParameters->VS.View = view.viewUniformBuffer;
	passParameters->VS.GPUScene = renderScene.GetGPUSceneParameters(m_renderGraph);
	passParameters->VS.PrimitivesToDraw = m_renderGraph.CreateSRV(primitivesToDraw);
	passParameters->PS.renderTargets.renderTargets[0] = colorTexture;
	passParameters->PS.renderTargets.depthTarget = depthTexture;
	passParameters->ProcessorParameters = m_meshPassProcessor->GetParameters(m_renderGraph);

	m_renderGraph.AddPass("Outline Geometry",
		RenderGraphPassFlags::Raster,
		passParameters,
		[passParameters, view, &renderScene, meshPassProcessor = m_meshPassProcessor](RenderContext& context)
	{
		BatchedShaderParameters batchedShaderParameters;
		context.CollectParameters(passParameters, batchedShaderParameters);
	
		RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->PS.renderTargets);

		context.BeginRendering(renderingInfo);
		meshPassProcessor->ExecuteCommands(context, batchedShaderParameters);
		context.EndRendering();
	});

	return colorTexture;
}

struct JumpFloodInitPS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(JumpFloodInitPS)
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, InputColor)
		SHADER_PARAMETER_SAMPLER(PointSampler)
		SHADER_PARAMETER(float2, RenderSize)
		RG_RENDER_TARGETS()
	END_SHADER_PARAMETER_STRUCT()
};
VT_REGISTER_SHADER(JumpFloodInitPS, "Engine/Shaders/Source/Editor/Outline/JumpFlood.hlsl", "JumpFloodInitPS", Pixel);

RGTextureRef OutlineTechnique::AddJumpFloodInitPass(RGTextureRef outlineGeometryImage, const Volt::RenderView& view)
{
	RGTextureRef jumpFloodInitTexture = m_renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "JumpFloodInit"));

	JumpFloodInitPS::Parameters* passParameters = m_renderGraph.AllocParameters<JumpFloodInitPS::Parameters>();
	passParameters->InputColor = m_renderGraph.CreateSRV(outlineGeometryImage);
	passParameters->PointSampler = SamplerStateCache::GetPointSampler();
	passParameters->RenderSize = { view.width, view.height };
	passParameters->renderTargets.renderTargets[0] = jumpFloodInitTexture;

	auto vertexShader = ShaderMap::Get<FullscreenTriangleVS>();
	auto pixelShader = ShaderMap::Get<JumpFloodInitPS>();

	m_renderGraph.AddPass("JumpFlood Init",
		RenderGraphPassFlags::Raster,
		passParameters,
		[passParameters, vertexShader, pixelShader, view](RenderContext& context) 
	{
		GraphicsPipelineState pipelineState{};
		pipelineState.shaders = { vertexShader, pixelShader };
		pipelineState.renderTargets = passParameters->renderTargets;

		RenderingInfo info = context.CreateRenderingInfo(view.width, view.height, passParameters->renderTargets);
	
		context.BeginRendering(info);
		context.SetPipelineState(pipelineState);
		context.SetParameters<JumpFloodInitPS>(pixelShader, passParameters);
		context.Draw(3, 1, 0, 0);
		context.EndRendering();
	});

	return jumpFloodInitTexture;
}

struct JumpFloodVS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(JumpFloodVS)
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER(float2, TexelSize)
		SHADER_PARAMETER(int, Step)
	END_SHADER_PARAMETER_STRUCT()
};
VT_REGISTER_SHADER(JumpFloodVS, "Engine/Shaders/Source/Editor/Outline/JumpFlood.hlsl", "JumpFloodPassVS", Vertex);

struct JumpFloodPS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(JumpFloodPS)
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, InputColor)
		SHADER_PARAMETER_SAMPLER(PointSampler)
		RG_RENDER_TARGETS()
	END_SHADER_PARAMETER_STRUCT()
};
VT_REGISTER_SHADER(JumpFloodPS, "Engine/Shaders/Source/Editor/Outline/JumpFlood.hlsl", "JumpFloodPassPS", Pixel);

BEGIN_SHADER_PARAMETER_STRUCT(JumpFloodParameters)
	SHADER_PARAMETER_STRUCT_INCLUDE(JumpFloodVS::Parameters, VS)
	SHADER_PARAMETER_STRUCT_INCLUDE(JumpFloodPS::Parameters, PS)
END_SHADER_PARAMETER_STRUCT()

RGTextureRef OutlineTechnique::AddJumpFloodPass(RGTextureRef prevImage, const Volt::RenderView& view, int32_t step)
{
	RGTextureRef jumpFloodTexture = m_renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "JumpFlood"));

	JumpFloodParameters* passParameters = m_renderGraph.AllocParameters<JumpFloodParameters>();
	passParameters->VS.Step = step;
	passParameters->VS.TexelSize = 1.f / glm::vec2(view.width, view.height);
	passParameters->PS.InputColor = m_renderGraph.CreateSRV(prevImage);
	passParameters->PS.PointSampler = SamplerStateCache::GetPointSampler();
	passParameters->PS.renderTargets.renderTargets[0] = jumpFloodTexture;

	auto vertexShader = ShaderMap::Get<JumpFloodVS>();
	auto pixelShader = ShaderMap::Get<JumpFloodPS>();

	m_renderGraph.AddPass("JumpFlood",
		RenderGraphPassFlags::Raster,
		passParameters,
		[passParameters, view, vertexShader, pixelShader](RenderContext& context)
	{
		GraphicsPipelineState pipelineState{};
		pipelineState.shaders = { vertexShader, pixelShader };
		pipelineState.renderTargets = passParameters->PS.renderTargets;

		RenderingInfo info = context.CreateRenderingInfo(view.width, view.height, passParameters->PS.renderTargets);

		context.BeginRendering(info);
		context.SetPipelineState(pipelineState);
		context.SetParameters<JumpFloodVS>(vertexShader, &passParameters->VS);
		context.SetParameters<JumpFloodPS>(pixelShader, &passParameters->PS);
		context.Draw(3, 1, 0, 0);
		context.EndRendering();
	});

	return jumpFloodTexture;
}

struct OutlineCompositeCS : GlobalShader
{
	DECLARE_GLOBAL_SHADER(OutlineCompositeCS)
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float4>, RWOutputColor)
		SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, JumpFloodResult)
		SHADER_PARAMETER(float3, OutlineColor)
		SHADER_PARAMETER(uint2, RenderSize)
	END_SHADER_PARAMETER_STRUCT()
};
VT_REGISTER_SHADER(OutlineCompositeCS, "Engine/Shaders/Source/Editor/Outline/OutlineComposite.hlsl", "OutlineCompositeCS", Compute);

void OutlineTechnique::AddOutlineCompositePass(RGTextureRef dstImage, const Volt::RenderView& view, RGTextureRef jumpfloodOutput)
{
	OutlineCompositeCS::Parameters* passParameters = m_renderGraph.AllocParameters<OutlineCompositeCS::Parameters>();
	passParameters->RWOutputColor = m_renderGraph.CreateUAV(dstImage);
	passParameters->JumpFloodResult = m_renderGraph.CreateSRV(jumpfloodOutput);
	passParameters->RenderSize = { view.width, view.height };
	passParameters->OutlineColor = glm::vec3(1.f, 0.5f, 0.f);

	auto computeShader = ShaderMap::Get<OutlineCompositeCS>();
	ComputeShaderUtils::AddPass<OutlineCompositeCS>(m_renderGraph,
		"Outline Composite",
		computeShader,
		passParameters,
		{ Math::DivideRoundUp(view.width, 8u), Math::DivideRoundUp(view.height, 8u), 1u });
}
