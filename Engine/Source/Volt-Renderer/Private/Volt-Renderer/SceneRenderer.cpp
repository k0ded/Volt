#include "vrpch.h"
#include "Volt-Renderer/SceneRenderer.h"

#include "Volt-Renderer/Camera/Camera.h"
#include "Volt-Renderer/RenderScene.h"
#include "Volt-Renderer/RendererCommon.h"
#include "Volt-Renderer/Renderer.h"
#include "Volt-Renderer/RayTracing/RayTracingScene.h"

#include "Volt-Renderer/RenderingTechniques/PrefixSumTechnique.h"
#include "Volt-Renderer/RenderingTechniques/GTAOTechnique.h"
#include "Volt-Renderer/RenderingTechniques/DirectionalShadowTechnique.h"
#include "Volt-Renderer/RenderingTechniques/LightCullingTechnique.h"
#include "Volt-Renderer/RenderingTechniques/ScreenSpaceReflections.h"
#include "Volt-Renderer/RenderingTechniques/AutoExposureTechnique.h"
#include "Volt-Renderer/RenderingTechniques/CullingTechnique.h"

#include "Volt-Renderer/ShapeLibrary.h"
#include "Volt-Renderer/Texture/Texture2D.h"

#include "Volt-Renderer/ShadowMappingUtility.h"

#include "Volt-Renderer/Mesh/Mesh.h"
#include "Volt-Renderer/Material.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphExecutionThread.h>
#include <RenderCore/RenderGraph/Resources/RenderGraphBufferResource.h>
#include <RenderCore/RenderGraph/Resources/RenderGraphTextureResource.h>
#include <RenderCore/RenderGraph/RenderContextUtils.h>
#include <RenderCore/RenderGraph/GPUReadbackBuffer.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/DefaultShaders.h>

#include <RHIModule/Images/Image.h>
#include <RHIModule/Shader/Shader.h>
#include <RHIModule/Pipelines/RenderPipeline.h>

#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	BEGIN_SHADER_PARAMETER_STRUCT(PathTracingParameters)
		SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<ViewData>, View)
		SHADER_PARAMETER_IMAGE(vt::RWTex2D<float4>, RWOutputTexture)
		SHADER_PARAMETER_STRUCT(GPUSceneData, GPUSceneData)
	END_SHADER_PARAMETER_STRUCT()

	BEGIN_SHADER_PARAMETER_STRUCT(VolumentricFogParameters)
		SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<VolumetricFogParams>, VolumetricFogParamsData)
		SHADER_PARAMETER_IMAGE(vt::Tex3D<float4>, IntegratedFogVolume)
		SHADER_PARAMETER_SAMPLER(vt::TextureSampler, PointSampler)
	END_SHADER_PARAMETER_STRUCT()

	BEGIN_SHADER_PARAMETER_STRUCT(SkyLight)
		SHADER_PARAMETER_IMAGE(vt::TexCube<float3>, irradiance)
		SHADER_PARAMETER_IMAGE(vt::TexCube<float3>, radiance)
	END_SHADER_PARAMETER_STRUCT()

	BEGIN_SHADER_PARAMETER_STRUCT(PBRConstants)
		SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<ViewData>, viewData)
		SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<DirectionalLightShadowData>, directionalLightShadowData)
		SHADER_PARAMETER_BUFFER(vt::TypedBuffer<LightDrawData>, lights)
		SHADER_PARAMETER_BUFFER(vt::TypedBuffer<uint>, visibleLights)
		SHADER_PARAMETER_SAMPLER(vt::TextureSampler, linearSampler)
		SHADER_PARAMETER_SAMPLER(vt::TextureSampler, pointLinearClampSampler)
		SHADER_PARAMETER_SAMPLER(vt::TextureSampler, shadowSampler)
		SHADER_PARAMETER_IMAGE(vt::Tex2D<float4>, DFGLuT)
		SHADER_PARAMETER_IMAGE(vt::Tex2DArray<float>, directionalLightShadowMap)
		SHADER_PARAMETER_STRUCT(SkyLight, skyLight)
	END_SHADER_PARAMETER_STRUCT()

	struct MaterialShaderTemp
	{
		BEGIN_SHADER_DEFINITION(MaterialShaderTemp)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Defaults/OpaqueDefault_cs.hlsl", "main", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<uint2>, VisibilityBuffer)
			SHADER_PARAMETER_BUFFER(vt::TypedBuffer<uint>, MaterialCountBuffer)
			SHADER_PARAMETER_BUFFER(vt::TypedBuffer<uint>, MaterialStartBuffer)
			SHADER_PARAMETER_BUFFER(vt::TypedBuffer<uint2>, PixelCollection)
			SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<ViewData>, View)
			SHADER_PARAMETER_IMAGE(vt::RWTex2D<float4>, Albedo)
			SHADER_PARAMETER_IMAGE(vt::RWTex2D<float3>, Normals)
			SHADER_PARAMETER_IMAGE(vt::RWTex2D<float2>, Material)
			SHADER_PARAMETER_IMAGE(vt::RWTex2D<float3>, Emissive)
			SHADER_PARAMETER(uint, MaterialId)
			SHADER_PARAMETER(float2, ViewSize)
			SHADER_PARAMETER_STRUCT(GPUSceneData, GPUSceneData)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(MaterialShaderTemp)

	struct DepthPrePassMSPS
	{
		BEGIN_SHADER_DEFINITION(DepthPrePassMSPS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/AmplificationCommon.hlsl", "MainAS", RHI::ShaderStage::Amplification)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/DepthPrePassMeshShader.hlsl", "MainMS", RHI::ShaderStage::Mesh)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/DepthPrePassMeshShader.hlsl", "MainPS", RHI::ShaderStage::Pixel)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_STRUCT_INCLUDE(MeshShaderCommonParameters, Common)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(DepthPrePassMSPS)

	struct ObjectIDMSPS
	{
		BEGIN_SHADER_DEFINITION(ObjectIDMSPS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/AmplificationCommon.hlsl", "MainAS", RHI::ShaderStage::Amplification)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/ObjectIDMeshShader.hlsl", "MainMS", RHI::ShaderStage::Mesh)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/ObjectIDMeshShader.hlsl", "MainPS", RHI::ShaderStage::Pixel)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_STRUCT_INCLUDE(MeshShaderCommonParameters, Common)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(ObjectIDMSPS)

	struct VisibilityBufferMSPS
	{
		BEGIN_SHADER_DEFINITION(VisibilityBufferMSPS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/AmplificationCommon.hlsl", "MainAS", RHI::ShaderStage::Amplification)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/VisibilityBufferMeshShader.hlsl", "MainMS", RHI::ShaderStage::Mesh)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/VisibilityBufferMeshShader.hlsl", "MainPS", RHI::ShaderStage::Pixel)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_STRUCT_INCLUDE(MeshShaderCommonParameters, Common)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(VisibilityBufferMSPS)

	struct GenerateMaterialCountCS
	{
		BEGIN_SHADER_DEFINITION(GenerateMaterialCountCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Visibility/GenerateMaterialCount_cs.hlsl", "main", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<uint2>, VisibilityBuffer)
			SHADER_PARAMETER_BUFFER(vt::RWTypedBuffer<uint>, MaterialCountsBuffer)
			SHADER_PARAMETER(uint2, RenderSize)
			SHADER_PARAMETER_STRUCT(GPUSceneData, GPUSceneData)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(GenerateMaterialCountCS)

	struct CollectMaterialPixelsCS
	{
		BEGIN_SHADER_DEFINITION(CollectMaterialPixelsCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Visibility/CollectMaterialPixels_cs.hlsl", "main", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<uint2>, VisibilityBuffer)
			SHADER_PARAMETER_BUFFER(vt::TypedBuffer<uint>, MaterialStartBuffer)
			SHADER_PARAMETER_BUFFER(vt::RWTypedBuffer<uint>, CurrentMaterialCountBuffer)
			SHADER_PARAMETER_BUFFER(vt::RWTypedBuffer<uint2>, PixelCollectionBuffer)
			SHADER_PARAMETER(uint2, RenderSize)
			SHADER_PARAMETER_STRUCT(GPUSceneData, GPUSceneData)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(CollectMaterialPixelsCS)

	struct GenerateMaterialIndirectArgsCS
	{
		BEGIN_SHADER_DEFINITION(GenerateMaterialIndirectArgsCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Visibility/GenerateMaterialIndirectArgs_cs.hlsl", "main", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER(vt::TypedBuffer<uint>, MaterialCounts)
			SHADER_PARAMETER_BUFFER(vt::RWTypedBuffer<uint>, RWIndirectArgsBuffer)
			SHADER_PARAMETER(uint, MaterialCount)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(GenerateMaterialIndirectArgsCS)

	struct SkyboxVSPS
	{
		BEGIN_SHADER_DEFINITION(SkyboxVSPS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Environment/Skybox_vs.hlsl", "main", RHI::ShaderStage::Vertex)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Environment/Skybox_ps.hlsl", "main", RHI::ShaderStage::Pixel)
		END_SHADER_DEFINITION()
	
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER(vt::TypedBuffer<VertexPositionData>, VertexPositions)
			SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<ViewData>, View)
			SHADER_PARAMETER_IMAGE(vt::TexCube<float3>, EnvironmentTexture)
			SHADER_PARAMETER_SAMPLER(vt::TextureSampler, LinearSampler)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float>, SceneDepth)
			SHADER_PARAMETER(float, LOD)
			SHADER_PARAMETER(float, Intensity)

			SHADER_PARAMETER_STRUCT_INCLUDE(VolumentricFogParameters, VolumetricFogParams)
			SHADER_PARAMETER_STRUCT(BlueNoiseShaderParameters, BlueNoise)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(SkyboxVSPS)

	struct ShadingCS
	{
		BEGIN_SHADER_DEFINITION(ShadingCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Shading/Shading_cs.hlsl", "main", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::RWTex2D<float4>, RWOutput)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float4>, Albedo)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float3>, Normals)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float2>, Material)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float3>, Emissive)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<uint>, AOTexture)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float>, DepthTexture)

			SHADER_PARAMETER_STRUCT_INCLUDE(VolumentricFogParameters, VolumetricFogParams)
			SHADER_PARAMETER_STRUCT(PBRConstants, PBRConstantsData)
			SHADER_PARAMETER_STRUCT(BlueNoiseShaderParameters, BlueNoise)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(ShadingCS)

	struct FXAAVSPS
	{
		BEGIN_SHADER_DEFINITION(FXAAVSPS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Utility/FullscreenTriangle_vs.hlsl", "main", RHI::ShaderStage::Vertex)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/PostProcessing/FXAA.hlsl", "main", RHI::ShaderStage::Pixel)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float3>, SceneColor)
			SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<ViewData>, View)
			SHADER_PARAMETER_SAMPLER(vt::TextureSampler, LinearSampler)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(FXAAVSPS)

	struct TonemapVSPS
	{
		BEGIN_SHADER_DEFINITION(TonemapVSPS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Utility/FullscreenTriangle_vs.hlsl", "main", RHI::ShaderStage::Vertex)
		DECLARE_SHADER_STAGE("Engine/Shaders/Source/PostProcessing/Tonemap.hlsl", "main", RHI::ShaderStage::Pixel)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float3>, FinalColor)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float>, AverageLuminance)
			SHADER_PARAMETER(float, MiddleGray)
			SHADER_PARAMETER(float, WhitePoint)
			SHADER_PARAMETER(uint, FrameIndex)

			SHADER_PARAMETER_STRUCT(BlueNoiseShaderParameters, BlueNoise)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(TonemapVSPS)

	struct VisualizationMS
	{
		BEGIN_SHADER_DEFINITION(VisualizationMS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/VisualizationMeshShader.hlsl", "MainAS", RHI::ShaderStage::Amplification)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/VisualizationMeshShader.hlsl", "MainMS", RHI::ShaderStage::Mesh)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/VisualizationMeshShader.hlsl", "MainPS", RHI::ShaderStage::Pixel)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER(uint, VisualizationModeInt)
			SHADER_PARAMETER_STRUCT_INCLUDE(MeshShaderCommonParameters, Common)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(VisualizationMS)

	struct VisualizationFullscreenCS
	{
		BEGIN_SHADER_DEFINITION(VisualizationMS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/RenderPipeline/VisualizationFullscreenShader.hlsl", "MainCS", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::RWTex2D<float4>, RWOutput)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float4>, Albedo)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float2>, Material)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float3>, SceneColor)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float>, SceneDepth)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float3>, SceneNormal)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<uint>, SceneAO)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float2>, Velocity)

			SHADER_PARAMETER(uint2, RenderSize)
			SHADER_PARAMETER(uint, VisualizationModeInt)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(VisualizationFullscreenCS)

	SceneRenderer::SceneRenderer(const SceneRendererCreateInfo& specification)
		: m_renderScene(specification.renderScene), m_commandBufferSet(Renderer::GetFramesInFlight())
	{
		CreateMainRenderTarget(specification.initialResolution.x, specification.initialResolution.y);

		RHI::ImageSpecification spec{};
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

	RefPtr<RHI::Image> SceneRenderer::GetObjectIDImage()
	{
		return m_objectIDImage;
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

		RenderGraphBlackboard blackboard{};
		RenderGraph renderGraph{ m_commandBufferSet.IncrementAndGetCommandBuffer() };

		renderGraph.SetTotalAllocatedSizeCallback([&](const uint64_t totalSize)
		{
			m_frameTotalGPUAllocation = totalSize;
		});
		
		if (ShouldApplyJitter())
		{
			m_prevJitter = m_currentJitter;
			m_currentJitter = m_taaNoise.Get(m_frameIndex, glm::uvec2(m_width, m_height));
			camera->SetSubpixelOffset(m_currentJitter);
		}

		m_renderScene->Update(renderGraph);

		SetupFrameData(renderGraph, blackboard, camera);

		UploadUniformBuffers(renderGraph, blackboard, camera);

		const uint32_t drawCount = m_renderScene->GetDrawCount();

		if (drawCount > 0)
		{
			AddMainCullingPass(renderGraph, blackboard);
			AddDepthPrePass(renderGraph, blackboard);
			AddObjectIDPass(renderGraph, blackboard);
			AddGTAOPass(renderGraph, blackboard, camera);

			DirectionalShadowTechnique dirShadowTechnique{ renderGraph, blackboard };
			blackboard.Add<DirectionalShadowData>() = dirShadowTechnique.Execute(camera, m_renderScene);

			LightCullingTechnique lightCulling{ renderGraph, blackboard };
			blackboard.Add<LightCullingData>() = lightCulling.Execute();

			blackboard.Add<VolumetricFogData>() = m_volumetricFog.Execute(renderGraph, blackboard);

			ExecuteGBufferGenerationPasses(renderGraph, blackboard);
			AddSkyboxPass(renderGraph, blackboard);
			
			AddShadingPass(renderGraph, blackboard);
			
			if (m_visualizationMode == VisualizationMode::None)
			{
				if (false)
				{
					AddPathTracingPass(renderGraph, blackboard, blackboard.Get<ShadingOutputData>().colorOutput);
				}
			
				//m_gibs.Render(renderGraph, blackboard, m_frameIndex);
				//m_ddgi.Render(renderGraph, rgBlackboard, m_scene->GetRenderScene());
			
				//ScreenSpaceReflections ssr(renderGraph, rgBlackboard);
				//ssr.Execute(rgBlackboard.Get<ShadingOutputData>().colorOutput);
			
				blackboard.Add<FinalOutput>().colorOutput = blackboard.Get<ShadingOutputData>().colorOutput;
				ExecutePostProcessingPasses(renderGraph, blackboard, timestep);
			}
			else
			{
				AddVisualizationPass(renderGraph, blackboard, renderGraph.AddExternalImage(m_outputImage));
			}
		}
		else
		{
			RGUtils::ClearImage(renderGraph, renderGraph.AddExternalImage(m_outputImage), { 0.1f, 0.1f, 0.1f, 1.f });
		}

		m_renderScene->EndFrame(renderGraph);

		{
			RenderGraphBarrierInfo barrier{};
			barrier.dstStage = RHI::BarrierStage::PixelShader;
			barrier.dstAccess = RHI::BarrierAccess::ShaderRead;
			barrier.dstLayout = RHI::ImageLayout::ShaderRead;

			renderGraph.AddResourceBarrier(renderGraph.AddExternalImage(m_outputImage), barrier);
		}

		m_renderGraphDebugger.ProcessRenderGraph(renderGraph);

		renderGraph.Compile();
		renderGraph.Execute();

		m_frameIndex++;
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

	void SceneRenderer::BuildMeshPass(RenderGraph::Builder& builder, RenderGraphBlackboard& blackboard)
	{
		const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();
		const auto& drawCullingData = blackboard.Get<DrawCullingData>();

		BuildGPUSceneData(builder, blackboard.Get<GPUSceneData>());

		builder.ReadResource(uniformBuffers.viewDataBuffer);
		builder.ReadResource(drawCullingData.countCommandBuffer, RenderGraphResourceState::IndirectArgument);
		builder.ReadResource(drawCullingData.taskCommandsBuffer);
	}

	void SetupMeshPassConstants(RenderContext& context, const RenderGraphBlackboard& blackboard, MeshShaderCommonParameters& parameters)
	{
		const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();
		const auto& drawCullingData = blackboard.Get<DrawCullingData>();
	
		parameters.GPUSceneData = blackboard.Get<GPUSceneData>();
		parameters.TaskCommands = drawCullingData.taskCommandsBuffer;
		parameters.View = uniformBuffers.viewDataBuffer;
	}

	//template<typename ShaderType>
	//void SetupMeshPassConstants(RenderContext& context, const RenderGraphBlackboard& blackboard)
	//{
	//	const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();
	//	const auto& drawCullingData = blackboard.Get<DrawCullingData>();
	//
	//	MeshShaderCommonParameters parameters;
	//	parameters.GPUSceneData = blackboard.Get<GPUSceneData>();
	//	parameters.TaskCommands = drawCullingData.taskCommandsBuffer;
	//	parameters.View = uniformBuffers.viewDataBuffer;
	//
	//	context.SetParameters<ShaderType>(parameters);
	//}

	void SceneRenderer::SetupFrameData(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, Ref<Camera> camera)
	{
		blackboard.Add<PreviousFrameData>() = m_previousFrameData;
		AddExternalResources(renderGraph, blackboard);
	}

	void SceneRenderer::UploadUniformBuffers(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, Ref<Camera> camera)
	{
		auto& buffersData = blackboard.Add<UniformBuffersData>();

		// View data
		{
			const auto desc = RGUtils::CreateBufferDesc<ViewUniformBuffer>(1, RHI::BufferUsage::StorageBuffer, RHI::MemoryUsage::CPUToGPU, "View Data");
			buffersData.viewDataBuffer = renderGraph.CreateUniformBuffer(desc);

			ViewUniformBuffer viewUniformBuffer{};

			// Camera
			viewUniformBuffer.projection = camera->GetProjection();
			viewUniformBuffer.view = camera->GetView();
			viewUniformBuffer.inverseView = glm::inverse(viewUniformBuffer.view);
			viewUniformBuffer.inverseProjection = glm::inverse(viewUniformBuffer.projection);
			viewUniformBuffer.viewProjection = viewUniformBuffer.projection * viewUniformBuffer.view;
			viewUniformBuffer.inverseViewProjection = glm::inverse(viewUniformBuffer.viewProjection);
			viewUniformBuffer.prevViewProjection = m_prevViewProjection;
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
			viewUniformBuffer.tileCountX = Math::DivideRoundUp(m_width, LightCullingTechnique::TILE_SIZE);
			viewUniformBuffer.lightCount = m_renderScene->GetLightCount();

			// Render Target
			viewUniformBuffer.renderSize = { m_width, m_height };
			viewUniformBuffer.invRenderSize = { 1.f / static_cast<float>(m_width), 1.f / static_cast<float>(m_height) };

			renderGraph.AddMappedBufferUpload(buffersData.viewDataBuffer, &viewUniformBuffer, sizeof(ViewUniformBuffer), "Upload view Uniform Buffer");

			blackboard.Add<ViewUniformBuffer>() = viewUniformBuffer;
		}

		// Directional light
		{
			const auto desc = RGUtils::CreateBufferDesc<DirectionalLightShadowUniformBuffer>(1, RHI::BufferUsage::StorageBuffer, RHI::MemoryUsage::CPUToGPU, "Directional Light Shadow Uniform Buffer");
			buffersData.directionalLightShadowDataBuffer = renderGraph.CreateUniformBuffer(desc);

			DirectionalLightInfo& lightInfo = blackboard.Add<DirectionalLightInfo>();
			DirectionalLightShadowUniformBuffer& data = lightInfo.data;

			for (const auto& light : m_renderScene->GetRenderLightData())
			{
				if (light.description.lightType == SceneLightType::Directional)
				{
					lightInfo.direction = light.description.direction;
					if (light.description.castShadows)
					{
						const Vector<float> cascades = { camera->GetFarPlane() / 50.f, camera->GetFarPlane() / 25.f, camera->GetFarPlane() / 10 };
						const auto lightMatrices = Utility::CalculateCascadeMatrices(camera, light.description.direction, cascades);

						for (size_t i = 0; i < lightMatrices.size(); i++)
						{
							const auto& matrices = lightMatrices.at(i);
							data.viewProjections[i] = matrices.projection * matrices.view;
							lightInfo.projectionBounds[i] = matrices.projectionBounds;
							lightInfo.views[i] = matrices.view;
						}

						for (size_t i = 0; i < cascades.size(); i++)
						{

							if (i < cascades.size())
							{
								data.cascadeDistances[i] = cascades.at(i);
							}
							else
							{
								data.cascadeDistances[i] = camera->GetFarPlane();
							}
						}
					}

					break;
				}
			}

			renderGraph.AddMappedBufferUpload(buffersData.directionalLightShadowDataBuffer, &data, sizeof(DirectionalLightShadowUniformBuffer), "Upload directional light data");
		}
	}

	void SceneRenderer::AddExternalResources(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		// Core images
		{
			auto& imageData = blackboard.Add<ExternalImagesData>();
			imageData.black1x1Cube = renderGraph.AddExternalImage(Renderer::GetDefaultResources().blackCubeTexture);
			imageData.white1x1 = renderGraph.AddExternalImage(Renderer::GetDefaultResources().whiteTexture->GetImage());
			imageData.DFGLuT = renderGraph.AddExternalImage(Renderer::GetDefaultResources().DFGLuT);
		}

		// Blue Noise
		{
			blackboard.Add<BlueNoiseTextures>() = BlueNoise::GetBlueNoiseTextures(renderGraph);
		}

		// GPU Scene
		{
			const auto gpuScene = m_renderScene->GetGPUSceneBuffers();

			auto& bufferData = blackboard.Add<GPUSceneData>();
			bufferData.meshesBuffer = renderGraph.AddExternalBuffer(gpuScene.meshesBuffer->GetResource());
			bufferData.sdfMeshesBuffer = renderGraph.AddExternalBuffer(gpuScene.sdfMeshesBuffer->GetResource());
			bufferData.materialsBuffer = renderGraph.AddExternalBuffer(gpuScene.materialsBuffer->GetResource());
			bufferData.primitiveDrawDataBuffer = renderGraph.AddExternalBuffer(gpuScene.primitiveDrawDataBuffer->GetResource());
			bufferData.prevPrimitiveDrawDataBuffer = renderGraph.AddExternalBuffer(gpuScene.prevPrimitiveDrawDataBuffer->GetResource());
			bufferData.validPrimitiveDrawDatasBuffer = renderGraph.AddExternalBuffer(gpuScene.validPrimitiveDrawDatasBuffer->GetResource());
			bufferData.bonesBuffer = renderGraph.AddExternalBuffer(gpuScene.bonesBuffer->GetResource());
			bufferData.lightsBuffer = renderGraph.AddExternalBuffer(gpuScene.lightsBuffer->GetResource());
		}

		const auto& imageData = blackboard.Get<ExternalImagesData>();
		
		auto& environmentTexturesData = blackboard.Add<EnvironmentTexturesData>();
		environmentTexturesData.irradiance = imageData.black1x1Cube;
		environmentTexturesData.radiance = imageData.black1x1Cube;
	
		for (const auto& light : m_renderScene->GetRenderLightData())
		{
			if (light.description.lightType == SceneLightType::Sky)
			{
				if (light.description.diffuseIBL)
				{
					environmentTexturesData.irradiance = renderGraph.AddExternalImage(light.description.diffuseIBL);
				}

				if (light.description.specularIBL)
				{
					environmentTexturesData.radiance = renderGraph.AddExternalImage(light.description.specularIBL);
				}

				break;
			}
		}
	}

	void SceneRenderer::ExecuteGBufferGenerationPasses(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		renderGraph.BeginMarker("Generate GBuffer");

		AddVisibilityBufferPass(renderGraph, blackboard);
		AddGenerateMaterialCountsPass(renderGraph, blackboard);

		PrefixSumTechnique prefixSum{ renderGraph };
		prefixSum.Execute(blackboard.Get<MaterialCountData>().materialCountBuffer, blackboard.Get<MaterialCountData>().materialStartBuffer, m_renderScene->GetIndividualMaterialCount());

		AddCollectMaterialPixelsPass(renderGraph, blackboard);
		AddGenerateMaterialIndirectArgsPass(renderGraph, blackboard);

		//For every material -> run compute shading shader using indirect args
		auto& gbufferData = blackboard.Add<GBufferData>();

		gbufferData.albedo = renderGraph.CreateImage(RGUtils::CreateImage2DDesc<RHI::PixelFormat::R8G8B8A8_UNORM>(m_width, m_height, RHI::ImageUsage::AttachmentStorage, "GBuffer.Albedo"));
		gbufferData.normals = renderGraph.CreateImage(RGUtils::CreateImage2DDesc<RHI::PixelFormat::B10G11R11_UFLOAT_PACK32>(m_width, m_height, RHI::ImageUsage::AttachmentStorage, "GBuffer.Normals"));
		gbufferData.material = renderGraph.CreateImage(RGUtils::CreateImage2DDesc<RHI::PixelFormat::R8G8_UNORM>(m_width, m_height, RHI::ImageUsage::AttachmentStorage, "GBuffer.Material"));
		gbufferData.emissive = renderGraph.CreateImage(RGUtils::CreateImage2DDesc<RHI::PixelFormat::B10G11R11_UFLOAT_PACK32>(m_width, m_height, RHI::ImageUsage::AttachmentStorage, "GBuffer.Emissive"));

		AddClearGBufferPass(renderGraph, blackboard);
		RenderMaterials(renderGraph, blackboard);

		renderGraph.EndMarker();
	}

	void SceneRenderer::ExecutePostProcessingPasses(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, float timestep)
	{
		renderGraph.BeginMarker("Post Processing");

		AutoExposureTechnique autoExposureTechnique(renderGraph, blackboard);
		autoExposureTechnique.Execute(blackboard.Get<FinalOutput>().colorOutput, renderGraph.AddExternalImage(m_averageLuminanceImage), timestep);

		if (m_antiAliasingMethod == AntiAliasingMethod::TAA)
		{
			TAATechnique taaTechnique(renderGraph, blackboard);
			TAAData taaData = taaTechnique.Execute(m_previousColorImage, blackboard.Get<DepthPrePass>().velocity);
			renderGraph.EnqueueImageExtraction(taaData.accumulationOutput, m_previousColorImage);
			blackboard.Get<FinalOutput>().colorOutput = taaData.taaOutput;
		}
		else
		{
			AddFXAAPass(renderGraph, blackboard, blackboard.Get<FinalOutput>().colorOutput);
			blackboard.Get<FinalOutput>().colorOutput = blackboard.Get<FXAAOutputData>().output;
		}

		AddTonemappingPass(renderGraph, blackboard, blackboard.Get<FinalOutput>().colorOutput);
		renderGraph.EndMarker();
	}

	void SceneRenderer::AddMainCullingPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		const auto& viewData = blackboard.Get<ViewUniformBuffer>();

		CullingTechnique cullingTechnique{ renderGraph, blackboard };

		CullingTechnique::Info info{};
		info.viewMatrix = viewData.view;
		info.cullingFrustum = viewData.cullingFrustum;
		info.nearPlane = viewData.nearPlane;
		info.farPlane = viewData.farPlane;
		info.drawCommandCount = m_renderScene->GetDrawCount();
		info.meshletCount = m_renderScene->GetMeshletCount();

		blackboard.Add<DrawCullingData>() = cullingTechnique.Execute(info);
	}

	void SceneRenderer::AddDepthPrePass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		const auto& drawCullingData = blackboard.Get<DrawCullingData>();

		blackboard.Add<DepthPrePass>() = renderGraph.AddPass<DepthPrePass>("Depth Pre Pass",
		[&](RenderGraph::Builder& builder, DepthPrePass& data)
		{
			RenderGraphImageDesc desc{};
			desc.width = m_width;
			desc.height = m_height;
			desc.format = RHI::PixelFormat::D32_SFLOAT;
			desc.usage = RHI::ImageUsage::Attachment;
			desc.name = "DepthPrePass.Depth";
			data.depth = builder.CreateImage(desc);

			desc.format = RHI::PixelFormat::R16G16B16A16_SFLOAT;
			desc.name = "DepthPrePass.ViewNormals";
			data.normals = builder.CreateImage(desc);

			desc.format = RHI::PixelFormat::R16G16_SFLOAT;
			desc.name = "DepthPrePass.Velocity";
			data.velocity = builder.CreateImage(desc);

			BuildMeshPass(builder, blackboard);
		},
		[=](const DepthPrePass& data, RenderContext& context)
		{
			RenderingInfo info = context.CreateRenderingInfo(m_width, m_height, { data.normals, data.velocity, data.depth });

			RHI::RenderPipelineCreateInfo pipelineInfo{};
			pipelineInfo.shader = ShaderMap::Get<DepthPrePassMSPS>();

			auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

			context.BeginRendering(info);
			context.BindPipeline(pipeline);

			DepthPrePassMSPS::Parameters parameters;
			SetupMeshPassConstants(context, blackboard, parameters.Common);

			context.SetParameters<DepthPrePassMSPS>(parameters);
			context.DispatchMeshTasksIndirect(drawCullingData.countCommandBuffer, sizeof(uint32_t), 1, 0);
			context.EndRendering();
		});
	}

	void SceneRenderer::AddObjectIDPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		struct Data
		{
			RenderGraphImageHandle objectIdHandle;
		};

		const auto preDepthHandle = blackboard.Get<DepthPrePass>().depth;
		const auto& drawCullingData = blackboard.Get<DrawCullingData>();

		Data& data = renderGraph.AddPass<Data>("Object ID Pass",
		[&](RenderGraph::Builder& builder, Data& data)
		{
			data.objectIdHandle = builder.CreateImage(RGUtils::CreateImage2DDesc<RHI::PixelFormat::R32_UINT>(m_width, m_height, RHI::ImageUsage::AttachmentStorage, "Entity ID"));
			builder.WriteResource(preDepthHandle);

			BuildMeshPass(builder, blackboard);
			builder.SetHasSideEffect();
		},
		[=](const Data& data, RenderContext& context)
		{
			RenderingInfo info = context.CreateRenderingInfo(m_width, m_height, { data.objectIdHandle, preDepthHandle });
			info.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;

			RHI::RenderPipelineCreateInfo pipelineInfo{};
			pipelineInfo.shader = ShaderMap::Get<ObjectIDMSPS>();
			pipelineInfo.depthCompareOperator = RHI::CompareOperator::Equal;
			pipelineInfo.depthMode = RHI::DepthMode::Read;

			auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

			context.BeginRendering(info);
			context.BindPipeline(pipeline);

			ObjectIDMSPS::Parameters parameters;
			SetupMeshPassConstants(context, blackboard, parameters.Common);

			context.SetParameters<ObjectIDMSPS>(parameters);

			context.DispatchMeshTasksIndirect(drawCullingData.countCommandBuffer, sizeof(uint32_t), 1, 0);
			context.EndRendering();
		});

		renderGraph.EnqueueImageExtraction(data.objectIdHandle, m_objectIDImage);
	}

	void SceneRenderer::AddGTAOPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, Ref<Camera> camera)
	{
		GTAOSettings tempSettings{};
		tempSettings.radius = 50.f;
		tempSettings.radiusMultiplier = 1.457f;
		tempSettings.falloffRange = 0.615f;
		tempSettings.finalValuePower = 2.2f;

		GTAOTechnique gtaoTechnique{ ShouldApplyJitter() ? m_frameIndex : 0, tempSettings };
		blackboard.Add<GTAOOutput>() = gtaoTechnique.Execute(renderGraph, blackboard, camera);
	}

	void SceneRenderer::AddVisibilityBufferPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		const auto preDepthHandle = blackboard.Get<DepthPrePass>().depth;
		const auto& drawCullingData = blackboard.Get<DrawCullingData>();

		blackboard.Add<VisibilityBufferData>() = renderGraph.AddPass<VisibilityBufferData>("Visibility Buffer",
		[&](RenderGraph::Builder& builder, VisibilityBufferData& data)
		{
			data.visibility = builder.CreateImage(RGUtils::CreateImage2DDesc<RHI::PixelFormat::R32G32_UINT>(m_width, m_height, RHI::ImageUsage::AttachmentStorage, "Visibility Buffer"));
			builder.WriteResource(preDepthHandle);

			BuildMeshPass(builder, blackboard);
		},
		[=](const VisibilityBufferData& data, RenderContext& context)
		{
			RenderingInfo info = context.CreateRenderingInfo(m_width, m_height, { data.visibility, preDepthHandle });
			info.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;
			info.renderingInfo.colorAttachments.At(0).SetClearColor(std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max());

			RHI::RenderPipelineCreateInfo pipelineInfo{};
			pipelineInfo.shader = ShaderMap::Get<VisibilityBufferMSPS>();
			pipelineInfo.depthCompareOperator = RHI::CompareOperator::Equal;
			pipelineInfo.depthMode = RHI::DepthMode::Read;

			auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

			context.BeginRendering(info);
			context.BindPipeline(pipeline);

			VisibilityBufferMSPS::Parameters parameters;
			SetupMeshPassConstants(context, blackboard, parameters.Common);

			context.SetParameters<VisibilityBufferMSPS>(parameters);

			context.DispatchMeshTasksIndirect(drawCullingData.countCommandBuffer, sizeof(uint32_t), 1, 0);
			context.EndRendering();
		});
	}

	void SceneRenderer::AddClearGBufferPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		const auto& gbufferData = blackboard.Get<GBufferData>();

		renderGraph.AddPass("Clear GBuffer",
		[&](RenderGraph::Builder& builder)
		{
			builder.WriteResource(gbufferData.albedo, RenderGraphResourceState::Clear);
			builder.WriteResource(gbufferData.normals, RenderGraphResourceState::Clear);
			builder.WriteResource(gbufferData.material, RenderGraphResourceState::Clear);
			builder.WriteResource(gbufferData.emissive, RenderGraphResourceState::Clear);
		},
		[=](RenderContext& context)
		{
			context.ClearImage(gbufferData.albedo, { 0.1f, 0.1f, 0.1f, 0.f });
			context.ClearImage(gbufferData.normals, { 0.f, 0.f, 0.f, 0.f });
			context.ClearImage(gbufferData.material, { 0.f, 0.f, 0.f, 0.f });
			context.ClearImage(gbufferData.emissive, { 0.f, 0.f, 0.f, 0.f });
		});
	}

	void SceneRenderer::AddGenerateMaterialCountsPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		const VisibilityBufferData& visBufferData = blackboard.Get<VisibilityBufferData>();
		const GPUSceneData& gpuSceneData = blackboard.Get<GPUSceneData>();

		RenderGraphBufferHandle materialCountBuffer = RenderGraphNullHandle();
		{
			const auto desc = RGUtils::CreateBufferDesc<uint32_t>(std::max(m_renderScene->GetIndividualMaterialCount(), 1u), RHI::BufferUsage::StorageBuffer, RHI::MemoryUsage::GPU, "Material Counts");
			materialCountBuffer = renderGraph.CreateBuffer(desc);
			RGUtils::ClearBuffer(renderGraph, materialCountBuffer, 0, "Clear Material Count");
		}

		blackboard.Add<MaterialCountData>() = renderGraph.AddPass<MaterialCountData>("Generate Material Count",
		[&](RenderGraph::Builder& builder, MaterialCountData& data)
		{
			{
				const auto desc = RGUtils::CreateBufferDesc<uint32_t>(std::max(m_renderScene->GetIndividualMaterialCount(), 1u), RHI::BufferUsage::StorageBuffer, RHI::MemoryUsage::GPU, "Material Starts");
				data.materialStartBuffer = builder.CreateBuffer(desc);
			}

			data.materialCountBuffer = materialCountBuffer;
			builder.WriteResource(data.materialCountBuffer);
			builder.ReadResource(visBufferData.visibility);

			BuildGPUSceneData(builder, gpuSceneData);

			builder.SetIsComputePass();
		},
		[=](const MaterialCountData& data, RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline<GenerateMaterialCountCS>();

			context.BindPipeline(pipeline);

			GenerateMaterialCountCS::Parameters parameters;
			parameters.VisibilityBuffer = visBufferData.visibility;
			parameters.MaterialCountsBuffer = data.materialCountBuffer;
			parameters.RenderSize = glm::uvec2(m_width, m_height);
			parameters.GPUSceneData = gpuSceneData;

			context.SetParameters<GenerateMaterialCountCS>(parameters);
			context.Dispatch(Math::DivideRoundUp(m_width, 8u), Math::DivideRoundUp(m_height, 8u), 1);
		});
	}

	void SceneRenderer::AddCollectMaterialPixelsPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		const VisibilityBufferData& visBufferData = blackboard.Get<VisibilityBufferData>();
		const MaterialCountData& matCountData = blackboard.Get<MaterialCountData>();
		const GPUSceneData& gpuSceneData = blackboard.Get<GPUSceneData>();

		RenderGraphBufferHandle currentMaterialCountBuffer = RenderGraphNullHandle();

		{
			const auto desc = RGUtils::CreateBufferDescGPU<uint32_t>(std::max(m_renderScene->GetIndividualMaterialCount(), 1u), "Current Material Count");
			currentMaterialCountBuffer = renderGraph.CreateBuffer(desc);
			RGUtils::ClearBuffer(renderGraph, currentMaterialCountBuffer, 0, "Clear Current Material Count");
		}

		blackboard.Add<MaterialPixelsData>() = renderGraph.AddPass<MaterialPixelsData>("Collect Material Pixels",
		[&](RenderGraph::Builder& builder, MaterialPixelsData& data)
		{
			{
				const auto desc = RGUtils::CreateBufferDescGPU<glm::uvec2>(m_width * m_height, "Pixel Collection");
				data.pixelCollectionBuffer = builder.CreateBuffer(desc);
			}

			data.currentMaterialCountBuffer = currentMaterialCountBuffer;

			builder.WriteResource(data.currentMaterialCountBuffer);

			BuildGPUSceneData(builder, gpuSceneData);

			builder.ReadResource(visBufferData.visibility);
			builder.ReadResource(matCountData.materialStartBuffer);

			builder.SetIsComputePass();
		},
		[=](const MaterialPixelsData& data, RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline<CollectMaterialPixelsCS>();

			context.BindPipeline(pipeline);

			CollectMaterialPixelsCS::Parameters parameters;
			parameters.VisibilityBuffer = visBufferData.visibility;
			parameters.MaterialStartBuffer = matCountData.materialStartBuffer;
			parameters.CurrentMaterialCountBuffer = data.currentMaterialCountBuffer;
			parameters.PixelCollectionBuffer = data.pixelCollectionBuffer;
			parameters.RenderSize = glm::uvec2(m_width, m_height);
			parameters.GPUSceneData = gpuSceneData;

			context.SetParameters<CollectMaterialPixelsCS>(parameters);
			context.Dispatch(Math::DivideRoundUp(m_width, 8u), Math::DivideRoundUp(m_height, 8u), 1);
		});
	}

	void SceneRenderer::AddGenerateMaterialIndirectArgsPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		MaterialCountData matCountData = blackboard.Get<MaterialCountData>();

		RenderGraphBufferHandle materialIndirectArgsBuffer = RenderGraphNullHandle();
		{
			const auto desc = RGUtils::CreateBufferDesc<RHI::IndirectDispatchCommand>(std::max(m_renderScene->GetIndividualMaterialCount(), 1u), RHI::BufferUsage::IndirectBuffer | RHI::BufferUsage::StorageBuffer, RHI::MemoryUsage::GPU, "Material Indirect Args");
			materialIndirectArgsBuffer = renderGraph.CreateBuffer(desc);
		}

		blackboard.Add<MaterialIndirectArgsData>() = renderGraph.AddPass<MaterialIndirectArgsData>("Generate Material Indirect Args",
		[&](RenderGraph::Builder& builder, MaterialIndirectArgsData& data)
		{
			data.materialIndirectArgsBuffer = materialIndirectArgsBuffer;

			builder.WriteResource(data.materialIndirectArgsBuffer);
			builder.ReadResource(matCountData.materialCountBuffer);
			builder.SetIsComputePass();
		},
		[=](const MaterialIndirectArgsData& data, RenderContext& context)
		{
			const uint32_t materialCount = m_renderScene->GetIndividualMaterialCount();

			auto pipeline = ShaderMap::GetComputePipeline<GenerateMaterialIndirectArgsCS>();

			GenerateMaterialIndirectArgsCS::Parameters parameters;
			parameters.MaterialCounts = matCountData.materialCountBuffer;
			parameters.RWIndirectArgsBuffer = data.materialIndirectArgsBuffer;
			parameters.MaterialCount = materialCount;

			context.BindPipeline(pipeline);
			context.SetParameters<GenerateMaterialIndirectArgsCS>(parameters);
			context.Dispatch(Math::DivideRoundUp(materialCount, 32u), 1, 1);
		});
	}

	void SceneRenderer::RenderMaterials(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		renderGraph.BeginMarker("Materials");

		for (uint32_t matId = 0; matId < m_renderScene->GetIndividualMaterialCount(); matId++)
		{
			AddGenerateGBufferPass(renderGraph, blackboard, matId);
		}

		renderGraph.EndMarker();
	}

	void SceneRenderer::AddGenerateGBufferPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const uint32_t materialId)
	{
		const auto& indirectArgsData = blackboard.Get<MaterialIndirectArgsData>();
		const auto& visBufferData = blackboard.Get<VisibilityBufferData>();
		const auto& matCountData = blackboard.Get<MaterialCountData>();
		const auto& matPixelsData = blackboard.Get<MaterialPixelsData>();
		const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();
		const auto& gpuSceneData = blackboard.Get<GPUSceneData>();

		const auto& gbufferData = blackboard.Get<GBufferData>();

		const std::string passName = std::format("Generate GBuffer Data: Material {0}", materialId);

		renderGraph.AddPass(passName,
		[&](RenderGraph::Builder& builder)
		{
			builder.ReadResource(indirectArgsData.materialIndirectArgsBuffer, RenderGraphResourceState::IndirectArgument);
			builder.ReadResource(visBufferData.visibility);
			builder.ReadResource(matCountData.materialCountBuffer);
			builder.ReadResource(matCountData.materialStartBuffer);
			builder.ReadResource(matPixelsData.pixelCollectionBuffer);
			builder.ReadResource(uniformBuffers.viewDataBuffer);

			BuildGPUSceneData(builder, gpuSceneData);

			builder.WriteResource(gbufferData.albedo);
			builder.WriteResource(gbufferData.normals);
			builder.WriteResource(gbufferData.material);
			builder.WriteResource(gbufferData.emissive);

			builder.SetIsComputePass();
		},
		[=](RenderContext& context)
		{
			auto material = m_renderScene->GetMaterialFromID(materialId);
			auto pipeline = material->GetPipeline();

			if (!pipeline)
			{
				pipeline = ShaderMap::GetComputePipeline<MaterialShaderTemp>();
			}

			context.BindPipeline(pipeline);

			MaterialShaderTemp::Parameters parameters;
			parameters.VisibilityBuffer = visBufferData.visibility;
			parameters.MaterialCountBuffer = matCountData.materialCountBuffer;
			parameters.MaterialStartBuffer = matCountData.materialStartBuffer;
			parameters.PixelCollection = matPixelsData.pixelCollectionBuffer;
			parameters.View = uniformBuffers.viewDataBuffer;
			parameters.Albedo = gbufferData.albedo;
			parameters.Normals = gbufferData.normals;
			parameters.Material = gbufferData.material;
			parameters.Emissive = gbufferData.emissive;
			parameters.MaterialId = materialId;
			parameters.ViewSize = glm::vec2(m_width, m_height);
			parameters.GPUSceneData = gpuSceneData;

			context.SetParameters<MaterialShaderTemp>(parameters);
			context.DispatchIndirect(indirectArgsData.materialIndirectArgsBuffer, sizeof(RHI::IndirectDispatchCommand) * materialId); // Should be offset with material ID
		});
	}

	void SceneRenderer::AddSkyboxPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		const auto& environmentTexturesData = blackboard.Get<EnvironmentTexturesData>();
		const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();
		const auto& volumetricFogData = blackboard.Get<VolumetricFogData>();
		const auto& depthPrePass = blackboard.Get<DepthPrePass>();
		const auto& blueNoiseTextures = blackboard.Get<BlueNoiseTextures>();

		RenderGraphBufferHandle meshVertexBufferHandle = renderGraph.AddExternalBuffer(m_skyboxMesh->GetVertexPositionsBuffer()->GetResource());
		RenderGraphBufferHandle indexBufferHandle = renderGraph.AddExternalBuffer(m_skyboxMesh->GetIndexBuffer()->GetResource());

		blackboard.Add<ShadingOutputData>() = renderGraph.AddPass<ShadingOutputData>("Skybox Pass",
		[&](RenderGraph::Builder& builder, ShadingOutputData& data)
		{
			{
				const auto desc = RGUtils::CreateImage2DDesc<RHI::PixelFormat::B10G11R11_UFLOAT_PACK32>(m_width, m_height, RHI::ImageUsage::AttachmentStorage, "Shading Output");
				data.colorOutput = builder.CreateImage(desc);
			}

			builder.ReadResource(environmentTexturesData.radiance);
			builder.ReadResource(meshVertexBufferHandle, RenderGraphResourceState::VertexBuffer);
			builder.ReadResource(indexBufferHandle, RenderGraphResourceState::IndexBuffer);
			builder.ReadResource(uniformBuffers.viewDataBuffer);
			builder.ReadResource(depthPrePass.depth);
			builder.ReadResource(volumetricFogData.fogParamsBuffer);
			builder.ReadResource(volumetricFogData.integratedFogVolume);

			BlueNoise::Build(builder, blueNoiseTextures);
		},
		[=](const ShadingOutputData& data, RenderContext& context)
		{
			RenderingInfo info = context.CreateRenderingInfo(m_width, m_height, { data.colorOutput });

			RHI::RenderPipelineCreateInfo pipelineInfo{};
			pipelineInfo.shader = ShaderMap::Get<SkyboxVSPS>();
			pipelineInfo.cullMode = RHI::CullMode::None;
			pipelineInfo.depthMode = RHI::DepthMode::None;

			auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

			const float lod = 0.f;
			const float intensity = 1.f;

			SkyboxVSPS::Parameters parameters;
			parameters.VertexPositions = meshVertexBufferHandle;
			parameters.View = uniformBuffers.viewDataBuffer;
			parameters.EnvironmentTexture = environmentTexturesData.radiance;
			parameters.LinearSampler = Renderer::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear>()->GetResourceHandle();
			parameters.SceneDepth = depthPrePass.depth;
			parameters.LOD = lod;
			parameters.Intensity = intensity;
			parameters.VolumetricFogParams.VolumetricFogParamsData = volumetricFogData.fogParamsBuffer;
			parameters.VolumetricFogParams.IntegratedFogVolume = volumetricFogData.integratedFogVolume;
			parameters.VolumetricFogParams.PointSampler = Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest>()->GetResourceHandle();

			BlueNoise::Setup(parameters.BlueNoise, blueNoiseTextures);

			context.BeginRendering(info);
			context.BindPipeline(pipeline);
			context.SetParameters<SkyboxVSPS>(parameters);
			context.BindIndexBuffer(indexBufferHandle);
			context.DrawIndexed(static_cast<uint32_t>(m_skyboxMesh->GetIndexCount()), 1, 0, 0, 0);
			context.EndRendering();
		});
	}

	void SceneRenderer::AddShadingPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();
		const auto& externalImages = blackboard.Get<ExternalImagesData>();
		const auto& environmentTexturesData = blackboard.Get<EnvironmentTexturesData>();
		const auto& shadingOutputData = blackboard.Get<ShadingOutputData>();
		const auto& gpuScene = blackboard.Get<GPUSceneData>();
		const auto& lightCullingData = blackboard.Get<LightCullingData>();
		const auto& gtaoOutput = blackboard.Get<GTAOOutput>();

		const auto& gbufferData = blackboard.Get<GBufferData>();
		const auto& preDepthData = blackboard.Get<DepthPrePass>();
		const auto& dirShadowData = blackboard.Get<DirectionalShadowData>();
		const auto& volumetricFogData = blackboard.Get<VolumetricFogData>();
		const auto& blueNoiseTextures = blackboard.Get<BlueNoiseTextures>();

		renderGraph.AddPass("Shading Pass",
		[&](RenderGraph::Builder& builder)
		{
			builder.WriteResource(shadingOutputData.colorOutput);
			builder.ReadResource(gbufferData.albedo);
			builder.ReadResource(gbufferData.normals);
			builder.ReadResource(gbufferData.material);
			builder.ReadResource(gbufferData.emissive);
			builder.ReadResource(preDepthData.depth);
			builder.ReadResource(volumetricFogData.fogParamsBuffer);
			builder.ReadResource(volumetricFogData.integratedFogVolume);

			// PBR Constants
			builder.ReadResource(uniformBuffers.viewDataBuffer);
			builder.ReadResource(uniformBuffers.directionalLightShadowDataBuffer);
			builder.ReadResource(externalImages.DFGLuT);
			builder.ReadResource(environmentTexturesData.irradiance);
			builder.ReadResource(environmentTexturesData.radiance);
			builder.ReadResource(gpuScene.lightsBuffer);
			builder.ReadResource(lightCullingData.visibleLightsBuffer);
			builder.ReadResource(dirShadowData.shadowTexture);
			builder.ReadResource(gtaoOutput.outputImage);

			BlueNoise::Build(builder, blueNoiseTextures);

			builder.SetIsComputePass();
		},
		[=](RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline<ShadingCS>();
			context.BindPipeline(pipeline);

			context.SetAccelerationStructure(m_renderScene->GetRayTracingScene()->GetAccelerationStructure());

			ShadingCS::Parameters parameters;
			parameters.RWOutput = shadingOutputData.colorOutput;
			parameters.Albedo = gbufferData.albedo;
			parameters.Normals = gbufferData.normals;
			parameters.Material = gbufferData.material;
			parameters.Emissive = gbufferData.emissive;
			parameters.AOTexture = gtaoOutput.outputImage;
			parameters.DepthTexture = preDepthData.depth;
			parameters.VolumetricFogParams.VolumetricFogParamsData = volumetricFogData.fogParamsBuffer;
			parameters.VolumetricFogParams.IntegratedFogVolume = volumetricFogData.integratedFogVolume;
			parameters.VolumetricFogParams.PointSampler = Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest>()->GetResourceHandle();

			parameters.PBRConstantsData.viewData = uniformBuffers.viewDataBuffer;
			parameters.PBRConstantsData.directionalLightShadowData = uniformBuffers.directionalLightShadowDataBuffer;
			parameters.PBRConstantsData.linearSampler = Renderer::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear>()->GetResourceHandle();
			parameters.PBRConstantsData.pointLinearClampSampler = Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Clamp>()->GetResourceHandle();
			parameters.PBRConstantsData.shadowSampler = Renderer::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Repeat, RHI::AnisotropyLevel::None, RHI::CompareOperator::LessEqual>()->GetResourceHandle();
			parameters.PBRConstantsData.DFGLuT = externalImages.DFGLuT;
			parameters.PBRConstantsData.lights = gpuScene.lightsBuffer;
			parameters.PBRConstantsData.visibleLights = lightCullingData.visibleLightsBuffer;
			parameters.PBRConstantsData.directionalLightShadowMap = dirShadowData.shadowTexture;
			parameters.PBRConstantsData.skyLight.irradiance = environmentTexturesData.irradiance;
			parameters.PBRConstantsData.skyLight.radiance = environmentTexturesData.radiance;

			BlueNoise::Setup(parameters.BlueNoise, blueNoiseTextures);

			context.SetParameters<ShadingCS>(parameters);
			context.Dispatch(Math::DivideRoundUp(m_width, 8u), Math::DivideRoundUp(m_height, 8u), 1u);
		});
	}

	void SceneRenderer::AddFXAAPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphImageHandle srcImage)
	{
		const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();

		blackboard.Add<FXAAOutputData>() = renderGraph.AddPass<FXAAOutputData>("FXAA Pass",
		[&](RenderGraph::Builder& builder, FXAAOutputData& data)
		{
			{
				const auto desc = RGUtils::CreateImage2DDesc<RHI::PixelFormat::B10G11R11_UFLOAT_PACK32>(m_width, m_height, RHI::ImageUsage::AttachmentStorage, "FXAA.Output");
				data.output = builder.CreateImage(desc);
			}

			builder.ReadResource(srcImage);
			builder.ReadResource(uniformBuffers.viewDataBuffer);

		},
		[=](const FXAAOutputData& data, RenderContext& context)
		{
			RenderingInfo info = context.CreateRenderingInfo(m_width, m_height, { data.output });

			RHI::RenderPipelineCreateInfo pipelineInfo;
			pipelineInfo.shader = ShaderMap::Get<FXAAVSPS>();
			pipelineInfo.depthMode = RHI::DepthMode::None;
			auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

			context.BeginRendering(info);

			FXAAVSPS::Parameters parameters;
			parameters.SceneColor = srcImage;
			parameters.View = uniformBuffers.viewDataBuffer;
			parameters.LinearSampler = Renderer::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear>()->GetResourceHandle();

			RCUtils::DrawFullscreenTriangle(context, pipeline, [&](RenderContext& context)
			{
				context.SetParameters<FXAAVSPS>(parameters);
			});

			context.EndRendering();
		});
	}

	void SceneRenderer::AddTonemappingPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphImageHandle srcImage)
	{
		constexpr float MiddleGray = 0.18f;
		constexpr float WhitePoint = 1.1f;

		RenderGraphImageHandle averageLuminanceImage = renderGraph.AddExternalImage(m_averageLuminanceImage);

		const auto& blueNoiseTextures = blackboard.Get<BlueNoiseTextures>();

		blackboard.Add<FinalCopyData>() = renderGraph.AddPass<FinalCopyData>("Tonemapping Pass",
		[&](RenderGraph::Builder& builder, FinalCopyData& data)
		{
			data.output = builder.AddExternalImage(m_outputImage);

			builder.WriteResource(data.output);
			builder.ReadResource(srcImage);
			builder.ReadResource(averageLuminanceImage);

			BlueNoise::Build(builder, blueNoiseTextures);
			builder.SetHasSideEffect();
		},
		[=](const FinalCopyData& data, RenderContext& context)
		{
			RenderingInfo info = context.CreateRenderingInfo(m_width, m_height, { data.output });

			RHI::RenderPipelineCreateInfo pipelineInfo;
			pipelineInfo.shader = ShaderMap::Get<TonemapVSPS>();
			pipelineInfo.depthMode = RHI::DepthMode::None;
			auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

			context.BeginRendering(info);

			TonemapVSPS::Parameters parameters;
			parameters.FinalColor = srcImage;
			parameters.AverageLuminance = averageLuminanceImage;
			parameters.MiddleGray = MiddleGray;
			parameters.WhitePoint = WhitePoint * WhitePoint;
			parameters.FrameIndex = m_frameIndex;
			BlueNoise::Setup(parameters.BlueNoise, blueNoiseTextures);

			RCUtils::DrawFullscreenTriangle(context, pipeline, [&](RenderContext& context)
			{
				context.SetParameters<TonemapVSPS>(parameters);
			});

			context.EndRendering();
		});
	}

	void SceneRenderer::AddVisualizationPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphImageHandle dstImage)
	{
		VisualizationMode visualizationMode = m_visualizationMode;

		if (IsMeshPassVisualizationMode())
		{
			const auto& drawCullingData = blackboard.Get<DrawCullingData>();
			const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();

			struct Data
			{
				RenderGraphImageHandle depthImage;
			};

			renderGraph.AddPass<Data>("Visualization Pass",
			[&](RenderGraph::Builder& builder, Data& data)
			{
				data.depthImage = builder.CreateImage(RGUtils::CreateImage2DDesc<RHI::PixelFormat::D32_SFLOAT>(m_width, m_height, RHI::ImageUsage::AttachmentStorage, "Visualization.Depth"));
				builder.WriteResource(dstImage);

				BuildMeshPass(builder, blackboard);
			},
			[=](const Data& data, RenderContext& context)
			{
				RenderingInfo info = context.CreateRenderingInfo(m_width, m_height, { dstImage, data.depthImage });

				RHI::RenderPipelineCreateInfo pipelineInfo{};
				pipelineInfo.shader = ShaderMap::Get<VisualizationMS>();

				auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);
				
				VisualizationMS::Parameters parameters;
				parameters.VisualizationModeInt = static_cast<uint32_t>(visualizationMode);
				parameters.Common.GPUSceneData = blackboard.Get<GPUSceneData>();
				parameters.Common.TaskCommands = drawCullingData.taskCommandsBuffer;
				parameters.Common.View = uniformBuffers.viewDataBuffer;

				context.BeginRendering(info);
				context.BindPipeline(pipeline);
				context.SetParameters<VisualizationMS>(parameters);
				context.DispatchMeshTasksIndirect(drawCullingData.countCommandBuffer, sizeof(uint32_t), 1, 0);
				context.EndRendering();
			});
		}
		else
		{
			const auto& gBufferData = blackboard.Get<GBufferData>();
			const auto& externalImages = blackboard.Get<ExternalImagesData>();
			const auto& shadingOutput = blackboard.Get<ShadingOutputData>();
			const auto& depthPrePass = blackboard.Get<DepthPrePass>();
			const auto& gtaoOutput = blackboard.Get<GTAOOutput>();

			renderGraph.AddPass("Visualization Pass",
			[=](RenderGraph::Builder& builder)
			{
				builder.WriteResource(dstImage);
				builder.ReadResource(externalImages.white1x1);

				if (visualizationMode == VisualizationMode::BaseColor)
				{
					builder.ReadResource(gBufferData.albedo);
				}
				else if (visualizationMode == VisualizationMode::Metallic || visualizationMode == VisualizationMode::Roughness)
				{
					builder.ReadResource(gBufferData.material);
				}
				else if (visualizationMode == VisualizationMode::SceneColor)
				{
					builder.ReadResource(shadingOutput.colorOutput);
				}
				else if (visualizationMode == VisualizationMode::SceneDepth)
				{
					builder.ReadResource(depthPrePass.depth);
				}
				else if (visualizationMode == VisualizationMode::WorldNormal)
				{
					builder.ReadResource(gBufferData.normals);
				}
				else if (visualizationMode == VisualizationMode::AmbientOcclusion)
				{
					builder.ReadResource(gtaoOutput.outputImage);
				}
				else if (visualizationMode == VisualizationMode::Velocity)
				{
					builder.ReadResource(depthPrePass.velocity);
				}

				builder.SetIsComputePass();
			},
			[=](RenderContext& context)
			{
				auto pipeline = ShaderMap::GetComputePipeline<VisualizationFullscreenCS>();

				VisualizationFullscreenCS::Parameters parameters;
				parameters.Albedo = visualizationMode == VisualizationMode::BaseColor ? gBufferData.albedo : externalImages.white1x1;
				parameters.Material = visualizationMode == VisualizationMode::Metallic || visualizationMode == VisualizationMode::Roughness ? gBufferData.material : externalImages.white1x1;
				parameters.SceneColor = visualizationMode == VisualizationMode::SceneColor ? shadingOutput.colorOutput : externalImages.white1x1;
				parameters.SceneDepth = visualizationMode == VisualizationMode::SceneDepth ? depthPrePass.depth : externalImages.white1x1;
				parameters.SceneNormal = visualizationMode == VisualizationMode::WorldNormal ? gBufferData.normals : externalImages.white1x1;
				parameters.SceneAO = visualizationMode == VisualizationMode::AmbientOcclusion ? gtaoOutput.outputImage : externalImages.white1x1;
				parameters.Velocity = visualizationMode == VisualizationMode::Velocity ? depthPrePass.velocity : externalImages.white1x1;
				parameters.RWOutput = dstImage;
				parameters.RenderSize = glm::uvec2(m_width, m_height);
				parameters.VisualizationModeInt = static_cast<uint32_t>(visualizationMode);

				context.BindPipeline(pipeline);
				context.SetParameters<VisualizationFullscreenCS>(parameters);
				context.Dispatch(Math::DivideRoundUp(m_width, 8u), Math::DivideRoundUp(m_height, 8u), 1u);
			});
		}
	}

	void SceneRenderer::AddPathTracingPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, RenderGraphImageHandle dstImage)
	{
		const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();
		const auto& gpuSceneData = blackboard.Get<GPUSceneData>();
		const auto& blueNoiseTextures = blackboard.Get<BlueNoiseTextures>();

		renderGraph.AddPass("RT Test Pass",
		[&](RenderGraph::Builder& builder)
		{
			builder.WriteResource(dstImage);

			builder.ReadResource(uniformBuffers.viewDataBuffer);
			builder.ReadResource(uniformBuffers.directionalLightShadowDataBuffer);

			BuildGPUSceneData(builder, gpuSceneData);
			BlueNoise::Build(builder, blueNoiseTextures);

			builder.SetIsRayTracingPass();
			builder.SetHasSideEffect();
		},
		[=](RenderContext& context)
		{
			RHI::RayTracingPipelineCreateInfo pipelineInfo;
			//pipelineInfo.rayGenTable.emplace_back(ShaderMap::Get("RayGen"));
			//pipelineInfo.missTable.emplace_back(ShaderMap::Get("Miss"));
			//pipelineInfo.missTable.emplace_back(ShaderMap::Get("MissShadow"));
			//pipelineInfo.closestHitTable.emplace_back(ShaderMap::Get("ClosestHit"));
			//pipelineInfo.closestHitTable.emplace_back(ShaderMap::Get("ClosestHitShadow"));

			auto pipeline = ShaderMap::GetRayTracingPipeline(pipelineInfo);
			auto sbt = ShaderMap::GetShaderBindingTable(pipeline);

			context.BindPipeline(pipeline);

			PathTracingParameters parameters;
			parameters.View = uniformBuffers.viewDataBuffer;
			parameters.RWOutputTexture = dstImage;
			parameters.GPUSceneData = gpuSceneData;
			
			//context.SetParameters(parameters);
			context.SetAccelerationStructure(m_renderScene->GetRayTracingScene()->GetAccelerationStructure());
			context.TraceRays(sbt, m_width, m_height, 1);
		});
	}

	void SceneRenderer::CreateMainRenderTarget(const uint32_t width, const uint32_t height)
	{
		RHI::ImageSpecification spec{};
		spec.width = width;
		spec.height = height;
		spec.usage = RHI::ImageUsage::AttachmentStorage;
		spec.generateMips = false;
		spec.format = RHI::PixelFormat::R8G8B8A8_UNORM;
		spec.debugName = "Final Image";

		m_outputImage = RHI::Image::Create(spec);
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
