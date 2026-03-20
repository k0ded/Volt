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
#include "Volt-Renderer/RenderView.h"
#include "Volt-Renderer/SystemTextures.h"
#include "Volt-Renderer/MainMaterialShaders.h"

#include "Volt-Renderer/Debug/DebugRenderer.h"

#include "Volt-Renderer/RenderingTechniques/TAATechnique.h"
#include "Volt-Renderer/RenderingTechniques/LightTileBinningTechnique.h"
#include "Volt-Renderer/RenderingTechniques/GTAOTechnique.h"
#include "Volt-Renderer/RenderingTechniques/CascadedShadowMapsTechnique.h"
#include "Volt-Renderer/RenderingTechniques/BloomTechnique.h"

#include "Volt-Renderer/MeshPassProcessors/DepthPrePassMeshProcessor.h"
#include "Volt-Renderer/MeshPassProcessors/BasePassMeshProcessor.h"
#include "Volt-Renderer/MeshPassProcessors/CascadedShadowMapsMeshProcessor.h"
#include "Volt-Renderer/MeshPassProcessors/TranslucencyMeshPassProcessor.h"

#include <JobSystem/JobSystem.h>

#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/GPUReadbackBuffer.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/DefaultShaders.h>
#include <RenderCore/Shader/BatchedShaderParameters.h>
#include <RenderCore/DefaultBlendStates.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

#include <RHIModule/Images/Image.h>
#include <RHIModule/Pipelines/RenderPipeline.h>

#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	VT_REGISTER_SHADER(TranslucencyCompositePS, "Engine/Shaders/Source/RenderPipelineLegacy/TranslucencyCompositePS.hlsl", "MainPS", Pixel);

	SceneRenderer::SceneRenderer(const SceneRendererCreateInfo& createInfo)
		: m_renderScene(createInfo.renderScene), m_createInfo(createInfo),
		m_meshPassProcessorRegistry(createInfo.renderScene.get())
	{
		CreateMainRenderTarget(createInfo.initialResolution.x, createInfo.initialResolution.y);

		RHI::ImageDesc spec{};
		spec.width = 1;
		spec.height = 1;
		spec.usage = RHI::ImageUsage::Storage;
		spec.format = RHI::PixelFormat::R16_SFLOAT;
		spec.debugName = "AutoExposure.AverageLuminance";

		m_averageLuminanceImage = RHI::Image::Create(spec);
		m_skyboxMesh = ShapeLibrary::GetCube();
	
		RegisterListener<AppPostFrameUpdateEvent>(VT_BIND_EVENT_FN(SceneRenderer::OnPostFrameUpdateEvent));

		AddMeshPassProcessors();
	}

	SceneRenderer::~SceneRenderer()
	{
		m_renderScene->GetRenderPrimitiveAddedDelegate().Remove(m_renderPrimitiveAddedDelegateHandle);
		m_renderScene->GetRenderPrimitiveRemovedDelegate().Remove(m_renderPrimitiveRemovedDelegateHandle);

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

	IntRef<RHI::Image> SceneRenderer::GetFinalImage()
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

		AddTranslucencyPass(renderGraph, blackboard, renderView, directionalShadowMap.shadowMap, directionalShadowMap.uniformBuffer);
		AddTranslucencyCompositePass(renderGraph, blackboard, renderView);

		m_globalIlluminationRenderer.Visualize(renderGraph, blackboard, renderView);

		AddPostProcessingPasses(renderGraph, blackboard, renderView, outputTexture);

		m_renderScene->RenderDebug(renderGraph, renderView, outputTexture, sceneTextures.sceneDepth);
		m_renderScene->EndFrame(renderGraph);


		renderGraph.Compile();
		m_renderGraphDebugger.ProcessRenderGraph(renderGraph);

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

	BEGIN_SHADER_PARAMETER_STRUCT(TranslucencyPassParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(TranslucencyPassVS::Parameters, VS)
		SHADER_PARAMETER_STRUCT_INCLUDE(TranslucencyPassMaterialShader::Parameters, PS)
		SHADER_PARAMETER_STRUCT_INCLUDE(MeshPassProcessorParameters, ProcessorParameters)
	END_SHADER_PARAMETER_STRUCT()

	void SceneRenderer::AddTranslucencyPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view, RGTextureRef directionalShadowMap, RGUniformBufferRef directionalShadowUniformBuffer)
	{
		VT_PROFILE_FUNCTION();

		TranslucencyTextures& translucencyTextures = blackboard.Add<TranslucencyTextures>();
		translucencyTextures.accumulation = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "Translucency.Accumulation"));
		translucencyTextures.revealage = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R8_UNORM>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "Translucency.Revealage"));

		const LightScene& lightScene = blackboard.Get<LightScene>();
		const EnvironmentTextures& environmentTextures = blackboard.Get<EnvironmentTextures>();
		const SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();

		m_translucencyMeshPassProcessor->PrepareRenderCommands(renderGraph);

		TranslucencyPassParameters* passParameters = renderGraph.AllocParameters<TranslucencyPassParameters>();
		passParameters->VS.View = view.viewUniformBuffer;
		passParameters->VS.GPUScene = m_renderScene->GetGPUSceneParameters(renderGraph);
		passParameters->PS.View = view.viewUniformBuffer;
		passParameters->PS.VisibleLightIndices = renderGraph.CreateSRV(lightScene.visibleLightIndices, RHI::PixelFormat::R32_SINT);

		passParameters->PS.DFGLuT = renderGraph.CreateSRV(environmentTextures.DFGLuT);
		passParameters->PS.SkylightIrradiance = renderGraph.CreateSRV(environmentTextures.irradiance);
		passParameters->PS.SkylightRadiance = renderGraph.CreateSRV(environmentTextures.radiance);
		passParameters->PS.LinearSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Clamp>();
		passParameters->PS.NumRadianceMipLevels = environmentTextures.radiance->GetDesc().mips;

		if (!directionalShadowMap)
		{
			directionalShadowMap = renderGraph.RegisterExternalTexture(Renderer::GetDefaultResources().blackCubeTexture);
		}

		passParameters->PS.CascadedDirectionalShadowMap = renderGraph.CreateSRV(directionalShadowMap);
		passParameters->PS.ShadowSampler = SamplerStateCache::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Repeat, RHI::AnisotropyLevel::None, RHI::CompareOperator::LessEqual>();
		passParameters->PS.CascadedDirectionalLightShadowMapping = directionalShadowUniformBuffer;
		passParameters->PS.renderTargets.renderTargets[0] = translucencyTextures.accumulation;
		passParameters->PS.renderTargets.renderTargets[1] = translucencyTextures.revealage;
		passParameters->PS.renderTargets.depthTarget = sceneTextures.sceneDepth;

		passParameters->ProcessorParameters = m_translucencyMeshPassProcessor->GetParameters(renderGraph);

		renderGraph.AddPass("TranslucencyPass",
			RenderGraphPassFlags::Raster,
			passParameters,
			[passParameters, view, meshPassProcessor = m_translucencyMeshPassProcessor](RenderContext& context)
		{
			BatchedShaderParameters batchedShaderParameters;
			context.CollectParameters(passParameters, batchedShaderParameters);

			RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->PS.renderTargets);
			renderingInfo.renderingInfo.colorAttachments[1].SetClearColor(1.f, 1.f, 1.f, 1.f);
			renderingInfo.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;

			context.BeginRendering(renderingInfo);
			meshPassProcessor->ExecuteCommands(context, batchedShaderParameters);
			context.EndRendering();
		});
	}

	void SceneRenderer::AddTranslucencyCompositePass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view)
	{
		const TranslucencyTextures& translucencyTextures = blackboard.Get<TranslucencyTextures>();
		const SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();

		TranslucencyCompositePS::Parameters* passParameters = renderGraph.AllocParameters<TranslucencyCompositePS::Parameters>();
		passParameters->Accumulation = renderGraph.CreateSRV(translucencyTextures.accumulation);
		passParameters->Revealage = renderGraph.CreateSRV(translucencyTextures.revealage);
		passParameters->renderTargets.renderTargets[0] = sceneTextures.sceneColor;

		auto vertexShader = ShaderMap::Get<FullscreenTriangleVS>();
		auto pixelShader = ShaderMap::Get<TranslucencyCompositePS>();

		renderGraph.AddPass("TranslucencyComposite",
			RenderGraphPassFlags::Raster,
			passParameters,
			[passParameters, view, pixelShader, vertexShader](RenderContext& context)
		{
			GraphicsPipelineState pipelineState{};
			pipelineState.shaders = { vertexShader, pixelShader };
			pipelineState.cullMode = RHI::CullMode::None;
			pipelineState.depthMode = RHI::DepthMode::None;
			pipelineState.attachmentBlendStates[0] = DefaultBlendStates::OneMinusSrcAlpha();
			pipelineState.renderTargets = passParameters->renderTargets;

			RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->renderTargets);
			renderingInfo.renderingInfo.colorAttachments[0].clearMode = RHI::ClearMode::Load;

			context.BeginRendering(renderingInfo);
			context.SetPipelineState(pipelineState);
			context.SetParameters<TranslucencyCompositePS>(pixelShader, passParameters);
			context.Draw(3, 1, 0, 0);
			context.EndRendering();
		});
	}

	void SceneRenderer::AddPostProcessingPasses(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view, RGTextureRef outputTexture)
	{
		renderGraph.BeginMarker("Post Processing");

		{
			BloomTechnique bloomTechnique{ renderGraph, blackboard };
			bloomTechnique.Execute(view);
		}

		if (m_antiAliasingMethod == AntiAliasingMethod::TAA)
		{
			TAATechnique taaTechnique(renderGraph, blackboard);
			TAATechnique::Output result = taaTechnique.Execute(view, m_previousColorImage);

			renderGraph.EnqueueTextureExtraction(result.accumulation, &m_previousColorImage);
		}

		renderGraph.EndMarker();

		SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();
		sceneTextures.sceneColor = ExecuteSceneRendererExtensions(SceneRendererExtensionStage::PostPostProcessing, renderGraph, blackboard, view, sceneTextures.sceneColor);

		AddTonemappingPass(renderGraph, blackboard, view, outputTexture);
	}

	void SceneRenderer::AddMeshPassProcessors()
	{
		m_renderPrimitiveAddedDelegateHandle = m_renderScene->GetRenderPrimitiveAddedDelegate().AddLambda([this](const RenderPrimitiveData* renderPrimitive)
		{
			m_meshPassProcessorRegistry.AddRenderPrimitive(renderPrimitive);
		});

		m_renderPrimitiveRemovedDelegateHandle = m_renderScene->GetRenderPrimitiveRemovedDelegate().AddLambda([this](const RenderPrimitiveData* renderPrimitive)
		{
			m_meshPassProcessorRegistry.RemoveRenderPrimitive(renderPrimitive);
		});

		m_depthPrePassMeshProcessor = m_meshPassProcessorRegistry.AddProcessor<DepthPrePassMeshProcessor>();
		m_basePassMeshProcessor = m_meshPassProcessorRegistry.AddProcessor<BasePassMeshProcessor>();
		m_cascadedShadowMapMeshProcessor = m_meshPassProcessorRegistry.AddProcessor<CascadedShadowMapMeshProcessor>();
		m_translucencyMeshPassProcessor = m_meshPassProcessorRegistry.AddProcessor<TranslucencyMeshPassProcessor>();
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
			SHADER_PARAMETER(uint, IsHDRMonitor)

			RG_RENDER_TARGETS()
			SHADER_PARAMETER_STRUCT_INCLUDE(BlueNoiseShaderParameters, BlueNoise)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(TonemapPS, "Engine/Shaders/Source/PostProcessing/Tonemap.hlsl", "MainPS", Pixel);

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
		passParameters->IsHDRMonitor = false; //WindowManager::Get().GetMainWindow().GetSwapchain().IsHDREnabled();
		passParameters->BlueNoise = BlueNoise::GetBlueNoiseParameters(renderGraph);
		passParameters->renderTargets.renderTargets[0] = outputTexture;

		auto vertexShader = ShaderMap::Get<FullscreenTriangleVS>();
		auto pixelShader = ShaderMap::Get<TonemapPS>();

		renderGraph.AddPass("Tonemap",
			RenderGraphPassFlags::Raster,
			passParameters,
			[passParameters, view, pixelShader, vertexShader](RenderContext& context)
		{
			GraphicsPipelineState pipelineState{};
			pipelineState.shaders = { vertexShader, pixelShader };
			pipelineState.cullMode = RHI::CullMode::None;
			pipelineState.depthMode = RHI::DepthMode::None;
			pipelineState.renderTargets = passParameters->renderTargets;

			RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->renderTargets);

			context.BeginRendering(renderingInfo);
			context.SetPipelineState(pipelineState);
			context.SetParameters<TonemapPS>(pixelShader, passParameters);
			context.Draw(3, 1, 0, 0);
			context.EndRendering();
		});
	}

	BEGIN_SHADER_PARAMETER_STRUCT(DepthPrePassParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(DepthPrePassVS::Parameters, VS)
		SHADER_PARAMETER_STRUCT_INCLUDE(MeshPassProcessorParameters, ProcessorParameters)
		SHADER_PARAMETER_UNIFORM_BUFFER(ViewData, View)
		RG_RENDER_TARGETS()
	END_SHADER_PARAMETER_STRUCT()

	void SceneRenderer::AddDepthPrePass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view)
	{
		VT_PROFILE_FUNCTION();

		SceneTextures& sceneTextures = blackboard.Add<SceneTextures>();
		sceneTextures.sceneVelocity = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16_SFLOAT>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "SceneVelocity"));
		sceneTextures.sceneDepth = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::D32_SFLOAT>(view.width, view.height, RHI::ImageUsage::Attachment, "SceneDepth"));
		
		m_depthPrePassMeshProcessor->PrepareRenderCommands(renderGraph);

		DepthPrePassParameters* passParameters = renderGraph.AllocParameters<DepthPrePassParameters>();
		passParameters->VS.View = view.viewUniformBuffer;
		passParameters->VS.GPUScene = m_renderScene->GetGPUSceneParameters(renderGraph);
		passParameters->ProcessorParameters = m_depthPrePassMeshProcessor->GetParameters(renderGraph);
		passParameters->View = view.viewUniformBuffer;
		passParameters->renderTargets.renderTargets[0] = sceneTextures.sceneVelocity;
		passParameters->renderTargets.depthTarget = sceneTextures.sceneDepth;

		renderGraph.AddPass("Depth Pre Pass",
			RenderGraphPassFlags::Raster,
			passParameters,
			[passParameters, view, meshPassProcessor = m_depthPrePassMeshProcessor](RenderContext& context)
		{
			BatchedShaderParameters batchedShaderParameters;
			context.CollectParameters(passParameters, batchedShaderParameters);

			RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->renderTargets);
			context.BeginRendering(renderingInfo);
			meshPassProcessor->ExecuteCommands(context, batchedShaderParameters);
			context.EndRendering();
		});
	}

	BEGIN_SHADER_PARAMETER_STRUCT(GenerateGBufferParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(BasePassVS::Parameters, VS)
		SHADER_PARAMETER_STRUCT_INCLUDE(MeshPassProcessorParameters, ProcessorParameters)
		RG_RENDER_TARGETS()
	END_SHADER_PARAMETER_STRUCT()

	void SceneRenderer::AddBasePass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view)
	{
		VT_PROFILE_FUNCTION();
		
		SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();
		sceneTextures.gBufferAlbedo = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R8G8B8A8_UNORM>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "GBufferAlbedo"));
		sceneTextures.gBufferNormals = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_UNORM>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "GBufferNormals"));
		sceneTextures.gBufferMaterial = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R8G8_UNORM>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "GBufferMaterial"));
		sceneTextures.gBufferEmissive = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::B10G11R11_UFLOAT_PACK32>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "GBufferEmissive"));

		m_basePassMeshProcessor->PrepareRenderCommands(renderGraph);

		GenerateGBufferParameters* passParameters = renderGraph.AllocParameters<GenerateGBufferParameters>();
		passParameters->VS.View = view.viewUniformBuffer;
		passParameters->VS.GPUScene = m_renderScene->GetGPUSceneParameters(renderGraph);
		passParameters->ProcessorParameters = m_basePassMeshProcessor->GetParameters(renderGraph);
		passParameters->renderTargets.renderTargets[0] = sceneTextures.gBufferAlbedo;
		passParameters->renderTargets.renderTargets[1] = sceneTextures.gBufferNormals;
		passParameters->renderTargets.renderTargets[2] = sceneTextures.gBufferMaterial;
		passParameters->renderTargets.renderTargets[3] = sceneTextures.gBufferEmissive;
		passParameters->renderTargets.depthTarget = sceneTextures.sceneDepth;

		renderGraph.AddPass("BasePass",
			RenderGraphPassFlags::Raster,
			passParameters,
			[passParameters, view, meshPassProcessor = m_basePassMeshProcessor](RenderContext& context)
		{
			BatchedShaderParameters batchedShaderParameters;
			context.CollectParameters(passParameters, batchedShaderParameters);

			RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->renderTargets);
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
	VT_REGISTER_SHADER(SkyboxVS, "Engine/Shaders/Source/RenderPipelineLegacy/Skybox.hlsl", "MainVS", Vertex);

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
	VT_REGISTER_SHADER(SkyboxPS, "Engine/Shaders/Source/RenderPipelineLegacy/Skybox.hlsl", "MainPS", Pixel);

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
			RenderGraphPassFlags::Raster,
			passParameters,
			[passParameters, view, vertexShader, pixelShader, indexCount] (RenderContext& context)
		{
			GraphicsPipelineState pipelineState{};
			pipelineState.shaders = { vertexShader, pixelShader };
			pipelineState.cullMode = RHI::CullMode::None;
			pipelineState.depthMode = RHI::DepthMode::None;
			pipelineState.renderTargets = passParameters->PS.renderTargets;

			RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->PS.renderTargets);
			renderingInfo.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;

			context.BeginRendering(renderingInfo);
			context.SetPipelineState(pipelineState);
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
	VT_REGISTER_SHADER(RenderDeferredShadingCS, "Engine/Shaders/Source/RenderPipelineLegacy/RenderDeferredShading.hlsl", "MainCS", Compute);

	struct CompositeLightingCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(CompositeLightingCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float4>, RWSceneColor)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, IndirectLight)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(CompositeLightingCS, "Engine/Shaders/Source/RenderPipelineLegacy/RenderDeferredShading.hlsl", "CompositeLightingCS", Compute);

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

			AddMappedBufferUploadCopyData(renderGraph, uniformBuffer, &viewUniformBuffer, sizeof(ViewUniformBuffer));

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
