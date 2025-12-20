#include "vrpch.h"
#include "Volt-Renderer/SceneRenderer.h"

#include "Volt-Renderer/Camera/Camera.h"
#include "Volt-Renderer/RenderScene.h"
#include "Volt-Renderer/RendererCommon.h"
#include "Volt-Renderer/Renderer.h"
#include "Volt-Renderer/RayTracing/RayTracingScene.h"
#include "Volt-Renderer/Utility/ScatteredBufferUpload.h"
#include "Volt-Renderer/ShapeLibrary.h"
#include "Volt-Renderer/Texture/Texture2D.h"
#include "Volt-Renderer/ShadowMappingUtility.h"
#include "Volt-Renderer/Mesh/Mesh.h"
#include "Volt-Renderer/SceneRendererRenderGraphData.h"
#include "Volt-Renderer/SceneRendererShaderDefinitions.h"
#include "Volt-Renderer/RenderView.h"
#include "Volt-Renderer/SystemTextures.h"

#include "Volt-Renderer/Debug/DebugRenderer.h"

#include "Volt-Renderer/RenderingTechniques/TAATechnique.h"
#include "Volt-Renderer/RenderingTechniques/LightTileBinningTechnique.h"
#include "Volt-Renderer/RenderingTechniques/GTAOTechnique.h"
#include "Volt-Renderer/RenderingTechniques/CascadedShadowMapsTechnique.h"

#include "Volt-Renderer/MeshPassProcessors/DepthPrePassMeshProcessor.h"
#include "Volt-Renderer/MeshPassProcessors/BasePassMeshProcessor.h"
#include "Volt-Renderer/MeshPassProcessors/CascadedShadowMapsMeshProcessor.h"

#include <JobSystem/JobSystem.h>
#include <AssetSystem/AssetLocks.h>

#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/GPUReadbackBuffer.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/DefaultShaders.h>
#include <RenderCore/Shader/BatchedShaderParameters.h>
#include <RenderCore/DefaultBlendStates.h>

#include <RHIModule/Images/Image.h>
#include <RHIModule/Pipelines/RenderPipeline.h>

#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	SceneRenderer::SceneRenderer(const SceneRendererCreateInfo& createInfo)
		: m_renderScene(createInfo.renderScene), m_createInfo(createInfo),
		m_meshPassProcessorRegistry(createInfo.renderScene.get())
	{
		CreateMainRenderTarget(createInfo.initialResolution.x, createInfo.initialResolution.y);

		RHI::ImageDesc spec{};
		spec.width = 1;
		spec.height = 1;
		spec.usage = RHI::ImageUsage::Storage;
		spec.generateMips = false;
		spec.format = RHI::PixelFormat::R16_SFLOAT;
		spec.debugName = "AutoExposure.AverageLuminance";

		m_averageLuminanceImage = RHI::Image::Create(spec);
		m_skyboxMesh = ShapeLibrary::GetCube();
	
		RegisterListener<AppPostFrameUpdateEvent>(VT_BIND_EVENT_FN(SceneRenderer::OnPostFrameUpdateEvent));

		AddMeshPassProcessors();
	}

	SceneRenderer::~SceneRenderer()
	{
		m_renderScene->UnregisterOnRenderPrimitiveAddedCallback(m_onRenderPrimitiveAddedCallbackId);
		m_renderScene->UnregisterOnRenderPrimitiveRemovedCallback(m_onRenderPrimitiveRemovedCallbackId);

		if (m_renderGraphExecutionCounter)
		{
			JobSystem::WaitForAndDestroyCounter(m_renderGraphExecutionCounter);
		}
	}

	void SceneRenderer::OnRenderEditor(Ref<Camera> camera, float timestep)
	{
		OnRender(camera, timestep);
	}

	void SceneRenderer::Resize(const uint32_t width, const uint32_t height)
	{
		m_resizeWidth = width;
		m_resizeHeight = height;

		m_shouldResize = true;
	}

	RefPtr<RHI::Image> SceneRenderer::GetFinalImage()
	{
		return m_outputImage;
	}

	void SceneRenderer::OnRender(Ref<Camera> camera, float timestep)
	{
		VT_PROFILE_FUNCTION();

		if (m_shouldResize)
		{
			m_width = m_resizeWidth;
			m_height = m_resizeHeight;

			// Make sure we wait for the previous frame to finish render before
			// resizing.
			if (m_renderGraphExecutionCounter)
			{
				JobSystem::WaitForAndDestroyCounter(m_renderGraphExecutionCounter);
			}

			CreateMainRenderTarget(m_width, m_height);
			m_shouldResize = false;
		}

		RenderGraphBlackboard blackboard;
		RenderGraph renderGraph{};

		SystemTextures::SetupSystemTextures(renderGraph, blackboard);

		RGTextureRef outputTexture = renderGraph.RegisterExternalTexture(m_outputImage);

		if (ShouldApplyJitter())
		{
			m_prevJitter = m_currentJitter;
			m_currentJitter = m_taaNoise.Get(m_frameIndex, { m_width, m_height });
			camera->SetSubpixelOffset(m_currentJitter);
		}

		m_renderScene->Update(renderGraph);

		RenderView renderView;
		renderView.width = m_width;
		renderView.height = m_height;
		renderView.viewUniformBuffer = CreateViewUniformBuffer(renderGraph, camera);
		renderView.frameIndex = m_frameIndex;
		renderView.camera = camera;
		renderView.renderScene = m_renderScene;

		AddDefaultTextures(renderGraph, blackboard);
		AddEnvironmentTextures(renderGraph, blackboard);
		AddDepthPrePass(renderGraph, blackboard, renderView);

		LightTileBinningTechnique tileBinningTechnique{ renderGraph, blackboard };
		tileBinningTechnique.Execute(renderView);

		ExecuteSceneRendererExtensions(SceneRendererExtensionStage::PreGBuffer, renderGraph, blackboard, renderView, nullptr);

		AddBasePass(renderGraph, blackboard, renderView);

		// Requires GBuffer normals.
		GTAOTechnique gtaoTechnique{ renderGraph, blackboard };
		gtaoTechnique.Execute(renderView);

		CascadedShadowMapsTechnique::Result directionalShadowMap{};

		for (const RenderLightData& light : m_renderScene->GetRenderLightData())
		{
			if (light.description.lightType == SceneLightType::Directional)
			{
				CascadedShadowMapsTechnique cascadedDirectionalShadowTechnique{ renderGraph, blackboard, m_cascadedShadowMapMeshProcessor };
				directionalShadowMap = cascadedDirectionalShadowTechnique.Execute(renderView, light);

				break;
			}
		}

		blackboard.Add<CascadedShadowMapsTechnique::Result>() = directionalShadowMap;

		SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();
		// Create shading RT
		{
			sceneTextures.sceneColor = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(renderView.width, renderView.height, RHI::ImageUsage::AttachmentStorage, "SceneColor"));
		}

		AddSkyboxPass(renderGraph, blackboard, renderView);

#if 0
		auto giOutput = m_globalIlluminationRenderer.Execute(renderGraph, blackboard, renderView);
#endif

		AddShadingPass(renderGraph, blackboard, renderView, directionalShadowMap.shadowMap, directionalShadowMap.uniformBuffer, nullptr);

		m_globalIlluminationRenderer.Visualize(renderGraph, blackboard, renderView);

		AddPostProcessingPasses(renderGraph, blackboard, renderView, outputTexture);

		if (m_createInfo.drawDebug)
		{
			Renderer::GetDebugRenderer().Render(renderGraph, renderView, outputTexture, sceneTextures.sceneDepth);
		}

		m_renderScene->EndFrame(renderGraph);

		{
			RHI::ResourceState barrier{};
			barrier.stage = RHI::BarrierStage::PixelShader;
			barrier.access = RHI::BarrierAccess::ShaderRead;
			barrier.layout = RHI::ImageLayout::ShaderRead;

			renderGraph.AddResourceBarrier(outputTexture, barrier);
		}

		//m_renderGraphDebugger.ProcessRenderGraph(renderGraph);

		renderGraph.Compile();
		m_renderGraphExecutionCounter = renderGraph.ExecuteAndExtractCounter();

		m_frameIndex++;
 	}

	void SceneRenderer::AddDefaultTextures(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		DefaultTextures& defaultTextures = blackboard.Add<DefaultTextures>();
		defaultTextures.white1x1 = renderGraph.RegisterExternalTexture(Renderer::GetDefaultResources().white1x1);
		defaultTextures.black1x1Cube = renderGraph.RegisterExternalTexture(Renderer::GetDefaultResources().blackCubeTexture);
	}

	void SceneRenderer::AddEnvironmentTextures(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		const DefaultTextures& defaultTextures = blackboard.Get<DefaultTextures>();

		EnvironmentTextures& environmentTextures = blackboard.Add<EnvironmentTextures>();
		environmentTextures.irradiance = defaultTextures.black1x1Cube;
		environmentTextures.radiance = defaultTextures.black1x1Cube;
		environmentTextures.DFGLuT = renderGraph.RegisterExternalTexture(Renderer::GetDefaultResources().DFGLuT);

		for (const RenderLightData& light : m_renderScene->GetRenderLightData())
		{
			if (light.description.lightType == SceneLightType::Sky)
			{
				if (light.description.diffuseIBL)
				{
					environmentTextures.irradiance = renderGraph.RegisterExternalTexture(light.description.diffuseIBL);
				}

				if (light.description.specularIBL)
				{
					environmentTextures.radiance = renderGraph.RegisterExternalTexture(light.description.specularIBL);
				}

				break;
			}
		}
	}

	void SceneRenderer::AddPostProcessingPasses(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view, RGTextureRef outputTexture)
	{
		renderGraph.BeginMarker("Post Processing");

		if (m_antiAliasingMethod == AntiAliasingMethod::TAA)
		{
			TAATechnique taaTechnique(renderGraph, blackboard);
			TAATechnique::Output result = taaTechnique.Execute(view, m_previousColorImage);

			renderGraph.EnqueueTextureExtraction(result.accumulation, &m_previousColorImage);
		}

		renderGraph.EndMarker();

		SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();

		ExecuteSceneRendererExtensions(SceneRendererExtensionStage::PostPostProcessing, renderGraph, blackboard, view, sceneTextures.sceneColor);

		AddTonemappingPass(renderGraph, blackboard, view, outputTexture);
	}

	void SceneRenderer::AddMeshPassProcessors()
	{
		m_depthPrePassMeshProcessor = m_meshPassProcessorRegistry.AddProcessor<DepthPrePassMeshProcessor>();
		m_basePassMeshProcessor = m_meshPassProcessorRegistry.AddProcessor<BasePassMeshProcessor>();
		m_cascadedShadowMapMeshProcessor = m_meshPassProcessorRegistry.AddProcessor<CascadedShadowMapMeshProcessor>();

		m_onRenderPrimitiveAddedCallbackId = m_renderScene->RegisterOnRenderPrimitiveAddedCallback([this](const RenderPrimitiveData& renderPrimitives)
		{
			m_meshPassProcessorRegistry.AddRenderPrimitive(renderPrimitives);
		});

		m_onRenderPrimitiveRemovedCallbackId = m_renderScene->RegisterOnRenderPrimitiveRemovedCallback([this](const RenderPrimitiveData& renderPrimitive)
		{
			m_meshPassProcessorRegistry.RemoveRenderPrimitive(renderPrimitive);
		});
	}

	bool SceneRenderer::OnPostFrameUpdateEvent(AppPostFrameUpdateEvent& event)
	{
		JobSystem::WaitForAndDestroyCounter(m_renderGraphExecutionCounter);
		return false;
	}

	RGTextureRef SceneRenderer::ExecuteSceneRendererExtensions(SceneRendererExtensionStage stage, RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view, RGTextureRef prevOutputImage)
	{
		RGTextureRef output = prevOutputImage;

		if (m_sceneRendererExtensions.contains(stage))
		{
			for (const auto& ext : m_sceneRendererExtensions.at(stage))
			{
				if (ext->ShouldRender())
				{
					output = ext->OnRender(renderGraph, blackboard, view, output);
				}
			}
		}

		return output;
	}

	struct TonemapPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(TonemapPS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float3>, FinalColor)
			SHADER_PARAMETER(float, MiddleGray)
			SHADER_PARAMETER(float, WhitePoint)
			SHADER_PARAMETER(uint, FrameIndex)

			RG_RENDER_TARGETS()
			SHADER_PARAMETER_STRUCT_INCLUDE(BlueNoiseShaderParameters, BlueNoise)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(TonemapPS, "Engine/Shaders/Source/PostProcessing/Tonemap.hlsl", "MainPS", Pixel);

	void SceneRenderer::AddTonemappingPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view, RGTextureRef outputTexture)
	{
		constexpr float MiddleGray = 0.18f;
		constexpr float WhitePoint = 1.1f;

		const SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();

		TonemapPS::Parameters* passParameters = renderGraph.AllocParameters<TonemapPS::Parameters>();
		passParameters->FinalColor = renderGraph.CreateSRV(sceneTextures.sceneColor);
		passParameters->MiddleGray = MiddleGray;
		passParameters->WhitePoint = WhitePoint * WhitePoint;
		passParameters->FrameIndex = view.frameIndex;
		passParameters->BlueNoise = BlueNoise::GetBlueNoiseParameters(renderGraph);
		passParameters->renderTargets.renderTargets[0] = outputTexture;

		auto vertexShader = ShaderMap::Get<FullscreenTriangleVS>();
		auto pixelShader = ShaderMap::Get<TonemapPS>();

		renderGraph.AddPass("Tonemap",
			RenderGraphPassFlags::None,
			passParameters,
			[passParameters, view, pixelShader, vertexShader](RenderContext& context)
		{
			RHI::RenderPipelineCreateInfo pipelineInfo{};
			pipelineInfo.shaders = { vertexShader, pixelShader };
			pipelineInfo.cullMode = RHI::CullMode::None;
			pipelineInfo.depthMode = RHI::DepthMode::None;

			auto pipeline = PipelineStateCache::GetRenderPipeline(pipelineInfo);

			RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->renderTargets);

			context.BeginRendering(renderingInfo);
			context.BindPipeline(pipeline);
			context.SetParameters<TonemapPS>(pixelShader, passParameters);
			context.Draw(3, 1, 0, 0);
			context.EndRendering();
		});
	}

	BEGIN_SHADER_PARAMETER_STRUCT(DepthPrePassParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(DepthPrePassVS::Parameters, VS)
		SHADER_PARAMETER_STRUCT_INCLUDE(DepthPrePassPS::Parameters, PS)
	END_SHADER_PARAMETER_STRUCT()

	void SceneRenderer::AddDepthPrePass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view)
	{
		VT_PROFILE_FUNCTION();

		SceneTextures& sceneTextures = blackboard.Add<SceneTextures>();
		sceneTextures.sceneVelocity = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16_SFLOAT>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "SceneVelocity"));
		sceneTextures.sceneDepth = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::D32_SFLOAT>(view.width, view.height, RHI::ImageUsage::Attachment, "SceneDepth"));

		DepthPrePassParameters* passParameters = renderGraph.AllocParameters<DepthPrePassParameters>();
		passParameters->VS.View = view.viewUniformBuffer;
		passParameters->VS.GPUScene = m_renderScene->GetGPUSceneParameters(renderGraph);
		passParameters->PS.View = view.viewUniformBuffer;
		passParameters->PS.renderTargets.renderTargets[0] = sceneTextures.sceneVelocity;
		passParameters->PS.renderTargets.depthTarget = sceneTextures.sceneDepth;

		m_depthPrePassMeshProcessor->PrepareRenderCommands(renderGraph);

		renderGraph.AddPass("Depth Pre Pass",
			RenderGraphPassFlags::None,
			passParameters,
			[passParameters, view, meshPassProcessor = m_depthPrePassMeshProcessor](RenderContext& context)
		{
			BatchedShaderParameters batchedShaderParameters;
			context.CollectParameters(passParameters, batchedShaderParameters);

			RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->PS.renderTargets);
			context.BeginRendering(renderingInfo);
			meshPassProcessor->ExecuteCommands(context, batchedShaderParameters);
			context.EndRendering();
		});
	}

	BEGIN_SHADER_PARAMETER_STRUCT(GenerateGBufferParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(BasePassVS::Parameters, VS)
		SHADER_PARAMETER_STRUCT_INCLUDE(BasePassPS::Parameters, PS)
	END_SHADER_PARAMETER_STRUCT()

	void SceneRenderer::AddBasePass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view)
	{
		VT_PROFILE_FUNCTION();
		
		SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();
		sceneTextures.gBufferAlbedo = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R8G8B8A8_UNORM>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "GBufferAlbedo"));
		sceneTextures.gBufferNormals = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_UNORM>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "GBufferNormals"));
		sceneTextures.gBufferMaterial = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R8G8_UNORM>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "GBufferMaterial"));
		sceneTextures.gBufferEmissive = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::B10G11R11_UFLOAT_PACK32>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "GBufferEmissive"));

		GenerateGBufferParameters* passParameters = renderGraph.AllocParameters<GenerateGBufferParameters>();
		passParameters->VS.View = view.viewUniformBuffer;
		passParameters->VS.GPUScene = m_renderScene->GetGPUSceneParameters(renderGraph);
		passParameters->PS.renderTargets.renderTargets[0] = sceneTextures.gBufferAlbedo;
		passParameters->PS.renderTargets.renderTargets[1] = sceneTextures.gBufferNormals;
		passParameters->PS.renderTargets.renderTargets[2] = sceneTextures.gBufferMaterial;
		passParameters->PS.renderTargets.renderTargets[3] = sceneTextures.gBufferEmissive;
		passParameters->PS.renderTargets.depthTarget = sceneTextures.sceneDepth;

		m_basePassMeshProcessor->PrepareRenderCommands(renderGraph);

		renderGraph.AddPass("BasePass",
			RenderGraphPassFlags::None,
			passParameters,
			[passParameters, view, meshPassProcessor = m_basePassMeshProcessor](RenderContext& context)
		{
			BatchedShaderParameters batchedShaderParameters;
			context.CollectParameters(passParameters, batchedShaderParameters);

			RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->PS.renderTargets);
			renderingInfo.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;

			context.BeginRendering(renderingInfo);
			meshPassProcessor->ExecuteCommands(context, batchedShaderParameters);
			context.EndRendering();
		});
	}

	struct SkyboxVS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(SkyboxVS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ViewData, View)
			RG_BUFFER_ACCESS(VertexBuffer, RGResourceAccess::VertexBuffer)
			RG_BUFFER_ACCESS(IndexBuffer, RGResourceAccess::IndexBuffer)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(SkyboxVS, "Engine/Shaders/Source/RenderPipelineLegacy/Skybox.hlsl", "MainVS", Vertex);

	struct SkyboxPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(SkyboxPS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_SRV(TextureCube<float3>, EnvironmentTexture)
			SHADER_PARAMETER_SAMPLER(LinearSampler)
			SHADER_PARAMETER(float, LOD)
			SHADER_PARAMETER(float, Intensity)

			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(SkyboxPS, "Engine/Shaders/Source/RenderPipelineLegacy/Skybox.hlsl", "MainPS", Pixel);

	BEGIN_SHADER_PARAMETER_STRUCT(SkyboxParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(SkyboxVS::Parameters, VS)
		SHADER_PARAMETER_STRUCT_INCLUDE(SkyboxPS::Parameters, PS)
	END_SHADER_PARAMETER_STRUCT()

	void SceneRenderer::AddSkyboxPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view)
	{
		VT_PROFILE_FUNCTION();
		
		const EnvironmentTextures& environmentTextures = blackboard.Get<EnvironmentTextures>();
		SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();

		SkyboxParameters* passParameters = renderGraph.AllocParameters<SkyboxParameters>();
		passParameters->VS.View = view.viewUniformBuffer;
		passParameters->VS.VertexBuffer = renderGraph.RegisterExternalBuffer(m_skyboxMesh->GetVertexPositionsBuffer());
		passParameters->VS.IndexBuffer = renderGraph.RegisterExternalBuffer(m_skyboxMesh->GetIndexBuffer());
		passParameters->PS.EnvironmentTexture = renderGraph.CreateSRV(environmentTextures.radiance);
		passParameters->PS.LinearSampler = SamplerStateCache::GetTrilinearSampler();
		passParameters->PS.LOD = 0.f;
		passParameters->PS.Intensity = 1.f;
		passParameters->PS.renderTargets.renderTargets[0] = sceneTextures.sceneColor;
		passParameters->PS.renderTargets.depthTarget = sceneTextures.sceneDepth;

		const uint32_t indexCount = static_cast<uint32_t>(m_skyboxMesh->GetIndexCount());

		auto vertexShader = ShaderMap::Get<SkyboxVS>();
		auto pixelShader = ShaderMap::Get<SkyboxPS>();

		renderGraph.AddPass("Skybox",
			RenderGraphPassFlags::None,
			passParameters,
			[passParameters, view, vertexShader, pixelShader, indexCount] (RenderContext& context)
		{
			RHI::RenderPipelineCreateInfo pipelineInfo{};
			pipelineInfo.shaders = { vertexShader, pixelShader };
			pipelineInfo.cullMode = RHI::CullMode::None;
			pipelineInfo.depthMode = RHI::DepthMode::None;

			auto pipeline = PipelineStateCache::GetRenderPipeline(pipelineInfo);

			RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->PS.renderTargets);
			renderingInfo.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;

			context.BeginRendering(renderingInfo);
			context.BindPipeline(pipeline);
			context.BindVertexBuffers({ passParameters->VS.VertexBuffer }, 0);
			context.BindIndexBuffer(passParameters->VS.IndexBuffer);
			context.SetParameters<SkyboxVS>(vertexShader, &passParameters->VS);
			context.SetParameters<SkyboxPS>(pixelShader, &passParameters->PS);
			context.DrawIndexed(indexCount, 1, 0, 0, 0);
			context.EndRendering();
		});
	}

	struct RenderDeferredShadingCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(RenderDeferredShadingCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ViewData, View)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, GBufferAlbedo)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, GBufferNormal)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float2>, GBufferMaterial)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float3>, GBufferEmissive)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float>, SceneDepth)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<uint>, SceneAO)
			SHADER_PARAMETER_BUFFER_SRV(Buffer<int>, VisibleLightIndices)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float4>, RWSceneColor)
			SHADER_PARAMETER_STRUCT_INCLUDE(GPUSceneParameters, GPUScene)

			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, DFGLuT)
			SHADER_PARAMETER_TEXTURE_SRV(TextureCube<float3>, SkylightIrradiance)
			SHADER_PARAMETER_TEXTURE_SRV(TextureCube<float3>, SkylightRadiance)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2DArray<float>, CascadedDirectionalShadowMap)
			SHADER_PARAMETER_UNIFORM_BUFFER(CascadedDirectionalLightShadowMappingData, CascadedDirectionalLightShadowMapping)
			SHADER_PARAMETER_SAMPLER(LinearSampler)
			SHADER_PARAMETER_SAMPLER(ShadowSampler)
			SHADER_PARAMETER(uint, NumRadianceMipLevels)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(RenderDeferredShadingCS, "Engine/Shaders/Source/RenderPipelineLegacy/RenderDeferredShading.hlsl", "MainCS", Compute);

	struct CompositeLightingCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(CompositeLightingCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float4>, RWSceneColor)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, IndirectLight)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(CompositeLightingCS, "Engine/Shaders/Source/RenderPipelineLegacy/RenderDeferredShading.hlsl", "CompositeLightingCS", Compute);

	void SceneRenderer::AddShadingPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view, 
		RGTextureRef directionalShadowMap, RGUniformBufferRef directionalShadowUniformBuffer, RGTextureRef indirectLightTexture)
	{
		VT_PROFILE_FUNCTION();

		const LightScene& lightScene = blackboard.Get<LightScene>();
		const EnvironmentTextures& environmentTextures = blackboard.Get<EnvironmentTextures>();
		SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();

		RGTextureUAVRef sceneColorUAV = renderGraph.CreateUAV(sceneTextures.sceneColor);

		{
			RenderDeferredShadingCS::Parameters* passParameters = renderGraph.AllocParameters<RenderDeferredShadingCS::Parameters>();
			passParameters->View = view.viewUniformBuffer;
			passParameters->VisibleLightIndices = renderGraph.CreateSRV(lightScene.visibleLightIndices, RHI::PixelFormat::R32_SINT);
			passParameters->GBufferAlbedo = renderGraph.CreateSRV(sceneTextures.gBufferAlbedo);
			passParameters->GBufferNormal = renderGraph.CreateSRV(sceneTextures.gBufferNormals);
			passParameters->GBufferMaterial = renderGraph.CreateSRV(sceneTextures.gBufferMaterial);
			passParameters->GBufferEmissive = renderGraph.CreateSRV(sceneTextures.gBufferEmissive);
			passParameters->SceneDepth = renderGraph.CreateSRV(sceneTextures.sceneDepth);
			passParameters->SceneAO = renderGraph.CreateSRV(sceneTextures.sceneAO);
			passParameters->GPUScene = m_renderScene->GetGPUSceneParameters(renderGraph);
			passParameters->RWSceneColor = sceneColorUAV;

			passParameters->DFGLuT = renderGraph.CreateSRV(environmentTextures.DFGLuT);
			passParameters->SkylightIrradiance = renderGraph.CreateSRV(environmentTextures.irradiance);
			passParameters->SkylightRadiance = renderGraph.CreateSRV(environmentTextures.radiance);
			passParameters->LinearSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Clamp>();
			passParameters->NumRadianceMipLevels = environmentTextures.radiance->GetDesc().mips;

			if (!directionalShadowMap)
			{
				directionalShadowMap = renderGraph.RegisterExternalTexture(Renderer::GetDefaultResources().blackCubeTexture);
			}

			passParameters->CascadedDirectionalShadowMap = renderGraph.CreateSRV(directionalShadowMap);
			passParameters->ShadowSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Repeat, RHI::AnisotropyLevel::None, RHI::CompareOperator::LessEqual>();
			passParameters->CascadedDirectionalLightShadowMapping = directionalShadowUniformBuffer;

			auto shader = ShaderMap::Get<RenderDeferredShadingCS>();
			ComputeShaderUtils::AddPass<RenderDeferredShadingCS>(renderGraph,
				"RenderDeferredShading",
				shader,
				passParameters,
				RenderGraphPassFlags::None,
				{ Math::DivideRoundUp(view.width, 8u), Math::DivideRoundUp(view.height, 8u), 1u });
		}

		{
			if (!indirectLightTexture)
			{
				indirectLightTexture = renderGraph.RegisterExternalTexture(Renderer::GetDefaultResources().black1x1);
			}

			CompositeLightingCS::Parameters* passParameters = renderGraph.AllocParameters<CompositeLightingCS::Parameters>();
			passParameters->RWSceneColor = sceneColorUAV;
			passParameters->IndirectLight = renderGraph.CreateSRV(indirectLightTexture);

			auto shader = ShaderMap::Get<CompositeLightingCS>();
			ComputeShaderUtils::AddPass<CompositeLightingCS>(renderGraph,
				"CompositeLighting",
				shader,
				passParameters,
				RenderGraphPassFlags::None,
				{ Math::DivideRoundUp(view.width, 8u), Math::DivideRoundUp(view.height, 8u), 1u });
		}
	}

	void SceneRenderer::Invalidate()
	{
		VT_PROFILE_FUNCTION();
	}

	void SceneRenderer::Enable()
	{
		m_enabled = true;
	}

	const uint64_t SceneRenderer::GetFrameTotalGPUAllocationSize() const
	{
		return m_frameTotalGPUAllocation.load();
	}

	void SceneRenderer::CreateMainRenderTarget(const uint32_t width, const uint32_t height)
	{
		RHI::ImageDesc spec{};
		spec.width = width;
		spec.height = height;
		spec.usage = RHI::ImageUsage::AttachmentStorage;
		spec.generateMips = false;
		spec.format = RHI::PixelFormat::B10G11R11_UFLOAT_PACK32;
		spec.debugName = "Final Image";
		spec.initializeImage = false;

		m_outputImage = RHI::Image::Create(spec);
	}

	RGUniformBufferRef SceneRenderer::CreateViewUniformBuffer(RenderGraph& renderGraph, Ref<Camera> camera)
	{
		VT_PROFILE_FUNCTION();

		// View data
		{
			RGUniformBufferRef uniformBuffer = renderGraph.CreateUniformBuffer(RGUniformBufferDesc::Create<ViewUniformBuffer>("View"));

			ViewUniformBuffer viewUniformBuffer{};

			// Camera
			viewUniformBuffer.projection = camera->GetProjection();
			viewUniformBuffer.view = camera->GetView();
			viewUniformBuffer.inverseView = glm::inverse(viewUniformBuffer.view);
			viewUniformBuffer.inverseProjection = glm::inverse(viewUniformBuffer.projection);
			viewUniformBuffer.viewProjection = viewUniformBuffer.projection * viewUniformBuffer.view;
			viewUniformBuffer.inverseViewProjection = glm::inverse(viewUniformBuffer.viewProjection);
			viewUniformBuffer.prevViewProjection = m_prevViewProjection;
			viewUniformBuffer.nonJitteredViewProjection = camera->GetNonJitteredProjection() * viewUniformBuffer.view;
			viewUniformBuffer.cameraPosition = glm::vec4(camera->GetPosition(), 1.f);
			viewUniformBuffer.nearPlane = camera->GetNearPlane();
			viewUniformBuffer.farPlane = camera->GetFarPlane();

			viewUniformBuffer.frameIndex = m_frameIndex;
			viewUniformBuffer.currentFrameJitter = glm::vec2(m_currentJitter.x, m_currentJitter.y);
			viewUniformBuffer.prevFrameJitter = glm::vec2(m_prevJitter.x, m_prevJitter.y);

			m_prevViewProjection = viewUniformBuffer.viewProjection;

			float depthLinearizeMul = (-viewUniformBuffer.projection[3][2]);
			float depthLinearizeAdd = (viewUniformBuffer.projection[2][2]);

			if (depthLinearizeMul * depthLinearizeAdd < 0.f)
			{
				depthLinearizeAdd = -depthLinearizeAdd;
			}

			viewUniformBuffer.depthUnpackConsts = { depthLinearizeMul, depthLinearizeAdd };
			viewUniformBuffer.cullingFrustum = camera->GetFrustumCullingInfo();

			// Light Culling
			viewUniformBuffer.tileCountX = Math::DivideRoundUp(m_width, LightTileBinningTechnique::TILE_SIZE);
			viewUniformBuffer.lightCount = m_renderScene->GetLightCount();

			// Render Target
			viewUniformBuffer.renderSize = { m_width, m_height };
			viewUniformBuffer.invRenderSize = { 1.f / static_cast<float>(m_width), 1.f / static_cast<float>(m_height) };

			AddMappedBufferUpload(renderGraph, uniformBuffer, &viewUniformBuffer, sizeof(ViewUniformBuffer));

			return uniformBuffer;
		}
	}

	bool SceneRenderer::ShouldApplyJitter() const
	{
		return m_antiAliasingMethod == AntiAliasingMethod::TAA && m_visualizationMode == VisualizationMode::None;
	}

	bool SceneRenderer::IsMeshPassVisualizationMode() const
	{
		return m_visualizationMode == VisualizationMode::GeometryNormals ||
			m_visualizationMode == VisualizationMode::UV ||
			m_visualizationMode == VisualizationMode::GeometryTangents;
	}
}
