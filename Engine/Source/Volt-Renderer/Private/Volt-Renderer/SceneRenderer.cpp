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

#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphExecutionThread.h>
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
	SceneRenderer::SceneRenderer(const SceneRendererCreateInfo& specification)
		: m_renderScene(specification.renderScene), m_commandBufferSet(Renderer::GetFramesInFlight())
	{
		CreateMainRenderTarget(specification.initialResolution.x, specification.initialResolution.y);

		RHI::ImageDesc spec{};
		spec.width = 1;
		spec.height = 1;
		spec.usage = RHI::ImageUsage::Storage;
		spec.generateMips = false;
		spec.format = RHI::PixelFormat::R16_SFLOAT;
		spec.debugName = "AutoExposure.AverageLuminance";

		m_averageLuminanceImage = RHI::Image::Create(spec);

		m_sceneEnvironment.specular = Renderer::GetDefaultResources().blackCubeTexture;
		m_sceneEnvironment.diffuse = Renderer::GetDefaultResources().blackCubeTexture;

		m_skyboxMesh = ShapeLibrary::GetCube();
	}

	SceneRenderer::~SceneRenderer()
	{
		RenderGraphExecutionThread::WaitForFinishedExecution();
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

			RenderGraphExecutionThread::WaitForFinishedExecution();

			CreateMainRenderTarget(m_width, m_height);
			m_shouldResize = false;
		}

		RenderGraphBlackboard blackboard;
		RenderGraph renderGraph{ m_commandBufferSet.IncrementAndGetCommandBuffer() };
		m_renderScene->Update(renderGraph);

		RenderView renderView;
		renderView.width = m_width;
		renderView.height = m_height;
		renderView.viewUniformBuffer = CreateViewUniformBuffer(renderGraph, camera);

		AddDefaultTextures(renderGraph, blackboard);
		AddEnvironmentTextures(renderGraph, blackboard);
		AddDepthPrePass(renderGraph, blackboard, renderView);
		AddGenerateGBufferPass(renderGraph, blackboard, renderView);

		m_renderScene->EndFrame(renderGraph);

		{
			RHI::ResourceState barrier{};
			barrier.stage = RHI::BarrierStage::PixelShader;
			barrier.access = RHI::BarrierAccess::ShaderRead;
			barrier.layout = RHI::ImageLayout::ShaderRead;

			renderGraph.AddResourceBarrier(renderGraph.RegisterExternalTexture(m_outputImage), barrier);
		}

		//m_renderGraphDebugger.ProcessRenderGraph(renderGraph);

		renderGraph.Compile();
		renderGraph.Execute();

		m_frameIndex++;
	}

	void SceneRenderer::AddDefaultTextures(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		DefaultTextures& defaultTextures = blackboard.Add<DefaultTextures>();
		defaultTextures.white1x1 = renderGraph.RegisterExternalTexture(Renderer::GetDefaultResources().whiteTexture->GetImage());
		defaultTextures.black1x1Cube = renderGraph.RegisterExternalTexture(Renderer::GetDefaultResources().blackCubeTexture);
	}

	void SceneRenderer::AddEnvironmentTextures(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		const DefaultTextures& defaultTextures = blackboard.Get<DefaultTextures>();

		EnvironmentTextures& environmentTextures = blackboard.Add<EnvironmentTextures>();
		environmentTextures.irradiance = defaultTextures.black1x1Cube;
		environmentTextures.radiance = defaultTextures.black1x1Cube;

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

	struct DepthPrePassVS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(DepthPrePassVS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
			SHADER_PARAMETER_STRUCT_INCLUDE(GPUSceneParameters, GPUScene)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(DepthPrePassVS, "Engine/Shaders/Source/RenderPipelineLegacy/DepthPrePass.hlsl", "MainVS", Vertex);

	struct DepthPrePassPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(DepthPrePassPS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(DepthPrePassPS, "Engine/Shaders/Source/RenderPipelineLegacy/DepthPrePass.hlsl", "MainPS", Pixel);

	BEGIN_SHADER_PARAMETER_STRUCT(DepthPrePassParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(DepthPrePassVS::Parameters, VS)
		SHADER_PARAMETER_STRUCT_INCLUDE(DepthPrePassPS::Parameters, PS)
	END_SHADER_PARAMETER_STRUCT()

	void SceneRenderer::AddDepthPrePass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view)
	{
		VT_PROFILE_FUNCTION();

		MeshRenderer meshRenderer;
		meshRenderer.BuildRenderCommands(m_renderScene, ShaderMap::Get<DepthPrePassVS>(), ShaderMap::Get<DepthPrePassPS>());

		SceneTextures& sceneTextures = blackboard.Add<SceneTextures>();
		sceneTextures.sceneVelocity = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16_SFLOAT>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "SceneVelocity"));
		sceneTextures.sceneDepth = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::D32_SFLOAT>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "SceneDepth"));

		DepthPrePassParameters* passParameters = renderGraph.AllocParameters<DepthPrePassParameters>();
		passParameters->VS.View = view.viewUniformBuffer;
		passParameters->VS.GPUScene = m_renderScene->GetGPUSceneParameters(renderGraph);
		passParameters->PS.View = view.viewUniformBuffer;
		passParameters->PS.renderTargets.renderTargets[0] = sceneTextures.sceneVelocity;
		passParameters->PS.renderTargets.depthTarget = sceneTextures.sceneDepth;

		renderGraph.AddPass("Depth Pre Pass",
			RenderGraphPassFlags::None,
			passParameters,
			[passParameters, view, meshRenderer](RenderContext& context)
		{
			BatchedShaderParameters batchedShaderParameters;
			context.CollectParameters(passParameters, batchedShaderParameters);

			RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->PS.renderTargets);
			context.BeginRendering(renderingInfo);

			meshRenderer.Render(context, batchedShaderParameters);

			context.EndRendering();
		});
	}

	struct GenerateGBufferVS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(GenerateGBufferVS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
			SHADER_PARAMETER_STRUCT_INCLUDE(GPUSceneParameters, GPUScene)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(GenerateGBufferVS, "Engine/Shaders/Source/RenderPipelineLegacy/GenerateGBuffer.hlsl", "MainVS", Vertex);

	struct GenerateGBufferPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(GenerateGBufferPS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(GenerateGBufferPS, "Engine/Shaders/Source/RenderPipelineLegacy/GenerateGBuffer.hlsl", "MainPS", Pixel);

	BEGIN_SHADER_PARAMETER_STRUCT(GenerateGBufferParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(GenerateGBufferVS::Parameters, VS)
		SHADER_PARAMETER_STRUCT_INCLUDE(GenerateGBufferPS::Parameters, PS)
	END_SHADER_PARAMETER_STRUCT()

	void SceneRenderer::AddGenerateGBufferPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view)
	{
		VT_PROFILE_FUNCTION();
		
		MeshRenderer meshRenderer;
		meshRenderer.BuildRenderCommands(m_renderScene, ShaderMap::Get<GenerateGBufferVS>(), ShaderMap::Get<GenerateGBufferPS>());

		SceneTextures& sceneTextures = blackboard.Get<SceneTextures>();
		sceneTextures.gBufferAlbedo = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R8G8B8A8_UNORM>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "GBufferAlbedo"));
		sceneTextures.gBufferNormals = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_UNORM>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "GBufferNormals"));
		sceneTextures.gBufferMaterial = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R8G8_UNORM>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "GBufferMaterial"));

		GenerateGBufferParameters* passParameters = renderGraph.AllocParameters<GenerateGBufferParameters>();
		passParameters->VS.View = view.viewUniformBuffer;
		passParameters->VS.GPUScene = m_renderScene->GetGPUSceneParameters(renderGraph);
		passParameters->PS.renderTargets.renderTargets[0] = sceneTextures.gBufferAlbedo;
		passParameters->PS.renderTargets.renderTargets[1] = sceneTextures.gBufferNormals;
		passParameters->PS.renderTargets.renderTargets[2] = sceneTextures.gBufferMaterial;
		passParameters->PS.renderTargets.depthTarget = sceneTextures.sceneDepth;

		renderGraph.AddPass("Generate GBuffer",
			RenderGraphPassFlags::NeverCull,
			passParameters,
			[passParameters, view, meshRenderer](RenderContext& context)
		{
			BatchedShaderParameters batchedShaderParameters;
			context.CollectParameters(passParameters, batchedShaderParameters);

			RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->PS.renderTargets);
			renderingInfo.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;

			context.BeginRendering(renderingInfo);
			meshRenderer.Render(context, batchedShaderParameters);
			context.EndRendering();
		});
	}

	void SceneRenderer::AddSkyboxPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view)
	{

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
		spec.format = RHI::PixelFormat::R8G8B8A8_UNORM;
		spec.debugName = "Final Image";

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
			viewUniformBuffer.currentFrameJitter = glm::vec2(m_currentJitter.x, -m_currentJitter.y);
			viewUniformBuffer.prevFrameJitter = glm::vec2(m_prevJitter.x, -m_prevJitter.y);

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
			//viewUniformBuffer.tileCountX = Math::DivideRoundUp(m_width, LightCullingTechnique::TILE_SIZE);
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
