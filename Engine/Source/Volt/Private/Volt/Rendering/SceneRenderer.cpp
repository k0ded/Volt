#include "vtpch.h"
#include "Volt/Rendering/SceneRenderer.h"

#include "Volt/Rendering/Camera/Camera.h"
#include "Volt/Rendering/RenderScene.h"
#include "Volt/Rendering/RendererCommon.h"
#include "Volt/Rendering/Renderer.h"
#include "Volt/Rendering/RayTracing/RayTracingScene.h"

#include "Volt/Rendering/RenderingTechniques/PrefixSumTechnique.h"
#include "Volt/Rendering/RenderingTechniques/GTAOTechnique.h"
#include "Volt/Rendering/RenderingTechniques/DirectionalShadowTechnique.h"
#include "Volt/Rendering/RenderingTechniques/LightCullingTechnique.h"
#include "Volt/Rendering/RenderingTechniques/ScreenSpaceReflections.h"
#include "Volt/Rendering/RenderingTechniques/AutoExposureTechnique.h"
#include "Volt/Rendering/RenderingTechniques/CullingTechnique.h"

#include "Volt/Rendering/ShapeLibrary.h"
#include "Volt/Rendering/Texture/Texture2D.h"

#include "Volt/Scene/Scene.h"
#include "Volt/Scene/Entity.h"

#include "Volt/Asset/Mesh/Mesh.h"
#include "Volt/Asset/Rendering/Material.h"

#include "Volt/Math/Math.h"

#include "Volt/Components/LightComponents.h"
#include "Volt/Components/RenderingComponents.h"

#include "Volt/Utility/ShadowMappingUtility.h"
#include "Volt/Utility/Noise.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphExecutionThread.h>
#include <RenderCore/RenderGraph/Resources/RenderGraphBufferResource.h>
#include <RenderCore/RenderGraph/Resources/RenderGraphTextureResource.h>
#include <RenderCore/RenderGraph/RenderContextUtils.h>
#include <RenderCore/RenderGraph/GPUReadbackBuffer.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <RHIModule/Images/Image.h>
#include <RHIModule/Shader/Shader.h>
#include <RHIModule/Pipelines/RenderPipeline.h>

namespace Volt
{
	SceneRenderer::SceneRenderer(const SceneRendererSpecification& specification)
		: m_scene(specification.scene), m_commandBufferSet(Renderer::GetFramesInFlight())
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

		m_scene->GetRenderScene()->Update(renderGraph);

		SetupFrameData(renderGraph, blackboard, camera);

		UploadUniformBuffers(renderGraph, blackboard, camera);
		UploadLightBuffers(renderGraph, blackboard);

		const auto renderScene = m_scene->GetRenderScene();
		const uint32_t drawCount = renderScene->GetDrawCount();

		if (drawCount > 0)
		{
			AddMainCullingPass(renderGraph, blackboard);
			AddDepthPrePass(renderGraph, blackboard);
			AddObjectIDPass(renderGraph, blackboard);
			AddGTAOPass(renderGraph, blackboard, camera);

			DirectionalShadowTechnique dirShadowTechnique{ renderGraph, blackboard };
			blackboard.Add<DirectionalShadowData>() = dirShadowTechnique.Execute(camera, m_scene->GetRenderScene());

			LightCullingTechnique lightCulling{ renderGraph, blackboard };
			blackboard.Add<LightCullingData>() = lightCulling.Execute();

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

		m_scene->GetRenderScene()->EndFrame(renderGraph);

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

		GPUSceneData::SetupInputs(builder, blackboard.Get<GPUSceneData>());

		builder.ReadResource(uniformBuffers.viewDataBuffer);
		builder.ReadResource(drawCullingData.countCommandBuffer, RenderGraphResourceState::IndirectArgument);
		builder.ReadResource(drawCullingData.taskCommandsBuffer);
	}

	void SceneRenderer::SetupMeshPassConstants(RenderContext& context, const RenderGraphBlackboard& blackboard)
	{
		const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();
		const auto& drawCullingData = blackboard.Get<DrawCullingData>();

		GPUSceneData::SetupConstants(context, blackboard.Get<GPUSceneData>());
		context.SetConstant("viewData"_sh, uniformBuffers.viewDataBuffer);
		context.SetConstant("taskCommands"_sh, drawCullingData.taskCommandsBuffer);
	}

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

			// Render Target
			viewUniformBuffer.renderSize = { m_width, m_height };
			viewUniformBuffer.invRenderSize = { 1.f / static_cast<float>(m_width), 1.f / static_cast<float>(m_height) };

			m_scene->ForEachWithComponents<const PointLightComponent, const IDComponent, const TransformComponent>([&](entt::entity entityId, const PointLightComponent& comp, const IDComponent& idComp, const TransformComponent& transComp)
			{
				if (!transComp.visible)
				{
					return;
				}

				viewUniformBuffer.pointLightCount++;
			});

			m_scene->ForEachWithComponents<const SpotLightComponent, const IDComponent, const TransformComponent>([&](entt::entity entityId, const SpotLightComponent& comp, const IDComponent& idComp, const TransformComponent& transComp)
			{
				if (!transComp.visible)
				{
					return;
				}

				viewUniformBuffer.spotLightCount++;
			});

			renderGraph.AddMappedBufferUpload(buffersData.viewDataBuffer, &viewUniformBuffer, sizeof(ViewUniformBuffer), "Upload view Uniform Buffer");

			blackboard.Add<ViewUniformBuffer>() = viewUniformBuffer;
		}

		// Directional light
		{
			const auto desc = RGUtils::CreateBufferDesc<DirectionalLightUniformBuffer>(1, RHI::BufferUsage::StorageBuffer, RHI::MemoryUsage::CPUToGPU, "Directional Light Uniform Buffer");
			buffersData.directionalLightBuffer = renderGraph.CreateUniformBuffer(desc);

			DirectionalLightInfo& lightInfo = blackboard.Add<DirectionalLightInfo>();
			DirectionalLightUniformBuffer& data = lightInfo.data;

			data.intensity = 0.f;

			m_scene->ForEachWithComponents<const DirectionalLightComponent, const IDComponent, const TransformComponent>([&](entt::entity id, const DirectionalLightComponent& dirLightComp, const IDComponent& idComp, const TransformComponent& comp)
			{
				if (!comp.visible)
				{
					return;
				}

				auto entity = m_scene->GetEntityFromID(idComp.id);
				const glm::vec3 dir = glm::rotate(entity.GetRotation(), { 0.f, 0.f, 1.f }) * -1.f;

				data.color = dirLightComp.color;
				data.intensity = glm::max(dirLightComp.intensity, 0.f);
				data.direction = dir;
				data.castShadows = static_cast<uint32_t>(dirLightComp.castShadows);
				data.angularRadius = glm::radians(dirLightComp.sunRadius);

				if (dirLightComp.castShadows)
				{
					const Vector<float> cascades = { camera->GetFarPlane() / 50.f, camera->GetFarPlane() / 25.f, camera->GetFarPlane() / 10 };
					const auto lightMatrices = Utility::CalculateCascadeMatrices(camera, dir, cascades);

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
			});

			renderGraph.AddMappedBufferUpload(buffersData.directionalLightBuffer, &data, sizeof(DirectionalLightUniformBuffer), "Upload directional light data");
		}
	}

	void SceneRenderer::UploadLightBuffers(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		auto& buffersData = blackboard.Add<LightBuffersData>();

		// Point lights
		{
			Vector<PointLightData> pointLights{};

			m_scene->ForEachWithComponents<const PointLightComponent, const IDComponent, const TransformComponent>([&](entt::entity entityId, const PointLightComponent& comp, const IDComponent& idComp, const TransformComponent& transComp)
			{
				if (!transComp.visible)
				{
					return;
				}

				auto entity = m_scene->GetEntityFromID(idComp.id);

				auto& data = pointLights.emplace_back();
				data.position = entity.GetPosition();
				data.radius = comp.radius;
				data.color = comp.color;
				data.intensity = comp.intensity * 100.f / 4 * glm::pi<float>(); // Convert from lm to cd, as we use CM we need to adjust the intensity, to match the units
				data.falloff = glm::clamp(comp.falloff, 0.f, 1.f);
			});

			const auto desc = RGUtils::CreateBufferDesc<PointLightData>(1, RHI::BufferUsage::StorageBuffer, RHI::MemoryUsage::CPUToGPU, "Point light Data");
			buffersData.pointLightsBuffer = renderGraph.CreateBuffer(desc);

			if (!pointLights.empty())
			{
				renderGraph.AddMappedBufferUpload(buffersData.pointLightsBuffer, pointLights.data(), sizeof(PointLightData) * pointLights.size(), "Upload point light data");
			}
		}

		// Spot lights
		{
			Vector<SpotLightData> spotLights{};

			m_scene->ForEachWithComponents<const SpotLightComponent, const IDComponent, const TransformComponent>([&](entt::entity entityId, const SpotLightComponent& comp, const IDComponent& idComp, const TransformComponent& transComp)
			{
				if (!transComp.visible)
				{
					return;
				}

				auto entity = m_scene->GetEntityFromID(idComp.id);

				const float cosInnerAngle = glm::cos(glm::radians(comp.innerAngle));
				const float cosOuterAngle = glm::cos(glm::radians(comp.outerAngle));

				auto& data = spotLights.emplace_back();
				data.position = entity.GetPosition();
				data.color = comp.color;
				data.falloff = comp.falloff;
				data.intensity = comp.intensity * 100.f / glm::pi<float>(); // Note: Not actually physically accurate, but easier to work with. As we use CM we need to adjust the intensity, to match the units
				data.direction = entity.GetForward() * -1.f;
				data.range = comp.range;
				data.lightAngleScale = 1.f / glm::max((cosInnerAngle - cosOuterAngle), 0.001f);
				data.lightAngleOffset = -cosOuterAngle * data.lightAngleScale;
			});

			const auto desc = RGUtils::CreateBufferDesc<SpotLightData>(1, RHI::BufferUsage::StorageBuffer, RHI::MemoryUsage::CPUToGPU, "Spot light Data");
			buffersData.spotLightsBuffer = renderGraph.CreateBuffer(desc);

			if (!spotLights.empty())
			{
				renderGraph.AddMappedBufferUpload(buffersData.spotLightsBuffer, spotLights.data(), sizeof(SpotLightData) * spotLights.size(), "Upload spot light data");
			}
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
			const auto gpuScene = m_scene->GetRenderScene()->GetGPUSceneBuffers();

			auto& bufferData = blackboard.Add<GPUSceneData>();
			bufferData.meshesBuffer = renderGraph.AddExternalBuffer(gpuScene.meshesBuffer->GetResource());
			bufferData.sdfMeshesBuffer = renderGraph.AddExternalBuffer(gpuScene.sdfMeshesBuffer->GetResource());
			bufferData.materialsBuffer = renderGraph.AddExternalBuffer(gpuScene.materialsBuffer->GetResource());
			bufferData.primitiveDrawDataBuffer = renderGraph.AddExternalBuffer(gpuScene.primitiveDrawDataBuffer->GetResource());
			bufferData.prevPrimitiveDrawDataBuffer = renderGraph.AddExternalBuffer(gpuScene.prevPrimitiveDrawDataBuffer->GetResource());
			bufferData.sdfPrimitiveDrawDataBuffer = renderGraph.AddExternalBuffer(gpuScene.sdfPrimitiveDrawDataBuffer->GetResource());
			bufferData.bonesBuffer = renderGraph.AddExternalBuffer(gpuScene.bonesBuffer->GetResource());
			bufferData.validPrimitiveDrawDatasBuffer = renderGraph.AddExternalBuffer(gpuScene.validPrimitiveDrawDatasBuffer->GetResource());
		}

		// Environment map
		{
			m_scene->ForEachWithComponents<SkylightComponent>([&](entt::entity id, SkylightComponent& skylightComp)
			{
				if (skylightComp.environmentHandle != skylightComp.lastEnvironmentHandle)
				{
					skylightComp.currentSceneEnvironment = Renderer::GenerateEnvironmentTextures(skylightComp.environmentHandle);
					skylightComp.lastEnvironmentHandle = skylightComp.environmentHandle;
				}

				m_sceneEnvironment = skylightComp.currentSceneEnvironment;
				m_sceneEnvironment.intensity = skylightComp.intensity;
				m_sceneEnvironment.lod = skylightComp.lod;
			});

			const auto& imageData = blackboard.Get<ExternalImagesData>();

			auto& environmentTexturesData = blackboard.Add<EnvironmentTexturesData>();
			environmentTexturesData.irradiance = m_sceneEnvironment.diffuse ? renderGraph.AddExternalImage(m_sceneEnvironment.diffuse) : imageData.black1x1Cube;
			environmentTexturesData.radiance = m_sceneEnvironment.specular ? renderGraph.AddExternalImage(m_sceneEnvironment.specular) : imageData.black1x1Cube;
		}
	}

	void SceneRenderer::ExecuteGBufferGenerationPasses(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		renderGraph.BeginMarker("Generate GBuffer");

		AddVisibilityBufferPass(renderGraph, blackboard);
		AddGenerateMaterialCountsPass(renderGraph, blackboard);

		PrefixSumTechnique prefixSum{ renderGraph };
		prefixSum.Execute(blackboard.Get<MaterialCountData>().materialCountBuffer, blackboard.Get<MaterialCountData>().materialStartBuffer, m_scene->GetRenderScene()->GetIndividualMaterialCount());

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
		const auto renderScene = m_scene->GetRenderScene();

		CullingTechnique cullingTechnique{ renderGraph, blackboard };

		CullingTechnique::Info info{};
		info.viewMatrix = viewData.view;
		info.cullingFrustum = viewData.cullingFrustum;
		info.nearPlane = viewData.nearPlane;
		info.farPlane = viewData.farPlane;
		info.drawCommandCount = renderScene->GetDrawCount();
		info.meshletCount = renderScene->GetMeshletCount();

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
			pipelineInfo.shader = ShaderMap::Get("DepthPrePassMeshShader");

			auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

			context.BeginRendering(info);
			context.BindPipeline(pipeline);

			SetupMeshPassConstants(context, blackboard);

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
			pipelineInfo.shader = ShaderMap::Get("ObjectIDMeshShader");
			pipelineInfo.depthCompareOperator = RHI::CompareOperator::Equal;
			pipelineInfo.depthMode = RHI::DepthMode::Read;

			auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

			context.BeginRendering(info);
			context.BindPipeline(pipeline);

			SetupMeshPassConstants(context, blackboard);

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
			pipelineInfo.shader = ShaderMap::Get("VisibilityBufferMeshShader");
			pipelineInfo.depthCompareOperator = RHI::CompareOperator::Equal;
			pipelineInfo.depthMode = RHI::DepthMode::Read;

			auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

			context.BeginRendering(info);
			context.BindPipeline(pipeline);

			SetupMeshPassConstants(context, blackboard);

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

		const auto& renderScene = m_scene->GetRenderScene();

		RenderGraphBufferHandle materialCountBuffer = RenderGraphNullHandle();
		{
			const auto desc = RGUtils::CreateBufferDesc<uint32_t>(std::max(renderScene->GetIndividualMaterialCount(), 1u), RHI::BufferUsage::StorageBuffer, RHI::MemoryUsage::GPU, "Material Counts");
			materialCountBuffer = renderGraph.CreateBuffer(desc);
			RGUtils::ClearBuffer(renderGraph, materialCountBuffer, 0, "Clear Material Count");
		}

		blackboard.Add<MaterialCountData>() = renderGraph.AddPass<MaterialCountData>("Generate Material Count",
		[&](RenderGraph::Builder& builder, MaterialCountData& data)
		{
			{
				const auto desc = RGUtils::CreateBufferDesc<uint32_t>(std::max(renderScene->GetIndividualMaterialCount(), 1u), RHI::BufferUsage::StorageBuffer, RHI::MemoryUsage::GPU, "Material Starts");
				data.materialStartBuffer = builder.CreateBuffer(desc);
			}

			data.materialCountBuffer = materialCountBuffer;
			builder.WriteResource(data.materialCountBuffer);
			builder.ReadResource(visBufferData.visibility);

			GPUSceneData::SetupInputs(builder, gpuSceneData);

			builder.SetIsComputePass();
		},
		[=](const MaterialCountData& data, RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline("GenerateMaterialCount");

			context.BindPipeline(pipeline);

			GPUSceneData::SetupConstants(context, gpuSceneData);

			context.SetConstant("visibilityBuffer"_sh, visBufferData.visibility);
			context.SetConstant("materialCountsBuffer"_sh, data.materialCountBuffer);
			context.SetConstant("renderSize"_sh, glm::uvec2{ m_width, m_height });

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
			const auto desc = RGUtils::CreateBufferDescGPU<uint32_t>(std::max(m_scene->GetRenderScene()->GetIndividualMaterialCount(), 1u), "Current Material Count");
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

			GPUSceneData::SetupInputs(builder, gpuSceneData);

			builder.ReadResource(visBufferData.visibility);
			builder.ReadResource(matCountData.materialStartBuffer);

			builder.SetIsComputePass();
		},
		[=](const MaterialPixelsData& data, RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline("CollectMaterialPixels");

			context.BindPipeline(pipeline);

			GPUSceneData::SetupConstants(context, gpuSceneData);

			context.SetConstant("visibilityBuffer"_sh, visBufferData.visibility);
			context.SetConstant("materialStartBuffer"_sh, matCountData.materialStartBuffer);
			context.SetConstant("currentMaterialCountBuffer"_sh, data.currentMaterialCountBuffer);
			context.SetConstant("pixelCollectionBuffer"_sh, data.pixelCollectionBuffer);
			context.SetConstant("renderSize"_sh, glm::uvec2{ m_width, m_height });

			context.Dispatch(Math::DivideRoundUp(m_width, 8u), Math::DivideRoundUp(m_height, 8u), 1);
		});
	}

	void SceneRenderer::AddGenerateMaterialIndirectArgsPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		MaterialCountData matCountData = blackboard.Get<MaterialCountData>();

		RenderGraphBufferHandle materialIndirectArgsBuffer = RenderGraphNullHandle();
		{
			const auto desc = RGUtils::CreateBufferDesc<RHI::IndirectDispatchCommand>(std::max(m_scene->GetRenderScene()->GetIndividualMaterialCount(), 1u), RHI::BufferUsage::IndirectBuffer | RHI::BufferUsage::StorageBuffer, RHI::MemoryUsage::GPU, "Material Indirect Args");
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
			const uint32_t materialCount = m_scene->GetRenderScene()->GetIndividualMaterialCount();

			auto pipeline = ShaderMap::GetComputePipeline("GenerateMaterialIndirectArgs");

			context.BindPipeline(pipeline);
			context.SetConstant("materialCounts"_sh, matCountData.materialCountBuffer);
			context.SetConstant("indirectArgsBuffer"_sh, data.materialIndirectArgsBuffer);
			context.SetConstant("materialCount"_sh, materialCount);

			context.Dispatch(Math::DivideRoundUp(materialCount, 32u), 1, 1);
		});
	}

	void SceneRenderer::RenderMaterials(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		renderGraph.BeginMarker("Materials");

		for (uint32_t matId = 0; matId < m_scene->GetRenderScene()->GetIndividualMaterialCount(); matId++)
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

		const auto& renderScene = m_scene->GetRenderScene();

		renderGraph.AddPass(passName,
		[&](RenderGraph::Builder& builder)
		{
			builder.ReadResource(indirectArgsData.materialIndirectArgsBuffer, RenderGraphResourceState::IndirectArgument);
			builder.ReadResource(visBufferData.visibility);
			builder.ReadResource(matCountData.materialCountBuffer);
			builder.ReadResource(matCountData.materialStartBuffer);
			builder.ReadResource(matPixelsData.pixelCollectionBuffer);
			builder.ReadResource(uniformBuffers.viewDataBuffer);

			GPUSceneData::SetupInputs(builder, gpuSceneData);

			builder.WriteResource(gbufferData.albedo);
			builder.WriteResource(gbufferData.normals);
			builder.WriteResource(gbufferData.material);
			builder.WriteResource(gbufferData.emissive);

			builder.SetIsComputePass();
		},
		[=](RenderContext& context)
		{
			auto material = renderScene->GetMaterialFromID(materialId);
			auto pipeline = material->GetComputePipeline();

			if (!pipeline)
			{
				pipeline = ShaderMap::GetComputePipeline("OpaqueDefault");
			}

			context.BindPipeline(pipeline);

			GPUSceneData::SetupConstants(context, gpuSceneData);

			context.SetConstant("visibilityBuffer"_sh, visBufferData.visibility);
			context.SetConstant("materialCountBuffer"_sh, matCountData.materialCountBuffer);
			context.SetConstant("materialStartBuffer"_sh, matCountData.materialStartBuffer);
			context.SetConstant("pixelCollection"_sh, matPixelsData.pixelCollectionBuffer);

			context.SetConstant("viewData"_sh, uniformBuffers.viewDataBuffer);

			context.SetConstant("albedo"_sh, gbufferData.albedo);
			context.SetConstant("normals"_sh, gbufferData.normals);
			context.SetConstant("material"_sh, gbufferData.material);
			context.SetConstant("emissive"_sh, gbufferData.emissive);
			context.SetConstant("materialId"_sh, materialId);

			context.SetConstant("viewSize"_sh, glm::vec2(m_width, m_height));

			context.DispatchIndirect(indirectArgsData.materialIndirectArgsBuffer, sizeof(RHI::IndirectDispatchCommand) * materialId); // Should be offset with material ID
		});
	}

	void SceneRenderer::AddSkyboxPass(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
	{
		const auto& environmentTexturesData = blackboard.Get<EnvironmentTexturesData>();
		const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();

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
		},
		[=](const ShadingOutputData& data, RenderContext& context)
		{
			RenderingInfo info = context.CreateRenderingInfo(m_width, m_height, { data.colorOutput });

			RHI::RenderPipelineCreateInfo pipelineInfo{};
			pipelineInfo.shader = ShaderMap::Get("Skybox");
			pipelineInfo.cullMode = RHI::CullMode::None;
			pipelineInfo.depthMode = RHI::DepthMode::None;

			auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

			context.BeginRendering(info);
			context.BindPipeline(pipeline);

			context.SetConstant("vertexPositions"_sh, meshVertexBufferHandle);
			context.SetConstant("viewData"_sh, uniformBuffers.viewDataBuffer);
			context.SetConstant("environmentTexture"_sh, environmentTexturesData.radiance);
			context.SetConstant("linearSampler"_sh, Renderer::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear>()->GetResourceHandle());
			context.SetConstant("lod"_sh, m_sceneEnvironment.lod);
			context.SetConstant("intensity"_sh, m_sceneEnvironment.intensity);

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
		const auto& lightBuffers = blackboard.Get<LightBuffersData>();
		const auto& lightCullingData = blackboard.Get<LightCullingData>();
		const auto& gtaoOutput = blackboard.Get<GTAOOutput>();

		const auto& gbufferData = blackboard.Get<GBufferData>();
		const auto& preDepthData = blackboard.Get<DepthPrePass>();
		const auto& dirShadowData = blackboard.Get<DirectionalShadowData>();

		renderGraph.AddPass("Shading Pass",
		[&](RenderGraph::Builder& builder)
		{
			builder.WriteResource(shadingOutputData.colorOutput);
			builder.ReadResource(gbufferData.albedo);
			builder.ReadResource(gbufferData.normals);
			builder.ReadResource(gbufferData.material);
			builder.ReadResource(gbufferData.emissive);
			builder.ReadResource(preDepthData.depth);

			// PBR Constants
			builder.ReadResource(uniformBuffers.viewDataBuffer);
			builder.ReadResource(uniformBuffers.directionalLightBuffer);
			builder.ReadResource(externalImages.DFGLuT);
			builder.ReadResource(environmentTexturesData.irradiance);
			builder.ReadResource(environmentTexturesData.radiance);
			builder.ReadResource(lightBuffers.pointLightsBuffer);
			builder.ReadResource(lightBuffers.spotLightsBuffer);
			builder.ReadResource(lightCullingData.visiblePointLightsBuffer);
			builder.ReadResource(lightCullingData.visibleSpotLightsBuffer);
			builder.ReadResource(dirShadowData.shadowTexture);
			builder.ReadResource(gtaoOutput.outputImage);

			builder.SetIsComputePass();
		},
		[=](RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline("Shading");
			context.BindPipeline(pipeline);

			//context.SetAccelerationStructure(m_scene->GetRenderScene()->GetRayTracingScene()->GetAccelerationStructure());

			context.SetConstant("output"_sh, shadingOutputData.colorOutput);
			context.SetConstant("albedo"_sh, gbufferData.albedo);
			context.SetConstant("normals"_sh, gbufferData.normals);
			context.SetConstant("material"_sh, gbufferData.material);
			context.SetConstant("emissive"_sh, gbufferData.emissive);
			context.SetConstant("aoTexture"_sh, gtaoOutput.outputImage);
			context.SetConstant("depthTexture"_sh, preDepthData.depth);

			// PBR Constants
			context.SetConstant("pbrConstants.viewData"_sh, uniformBuffers.viewDataBuffer);
			context.SetConstant("pbrConstants.directionalLight"_sh, uniformBuffers.directionalLightBuffer);
			context.SetConstant("pbrConstants.linearSampler"_sh, Renderer::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear>()->GetResourceHandle());
			context.SetConstant("pbrConstants.pointLinearClampSampler"_sh, Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Clamp>()->GetResourceHandle());
			context.SetConstant("pbrConstants.shadowSampler"_sh, Renderer::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Repeat, RHI::AnisotropyLevel::None, RHI::CompareOperator::LessEqual>()->GetResourceHandle());
			context.SetConstant("pbrConstants.DFGLuT"_sh, externalImages.DFGLuT);
			context.SetConstant("pbrConstants.pointLights"_sh, lightBuffers.pointLightsBuffer);
			context.SetConstant("pbrConstants.spotLights"_sh, lightBuffers.spotLightsBuffer);
			context.SetConstant("pbrConstants.visiblePointLights"_sh, lightCullingData.visiblePointLightsBuffer);
			context.SetConstant("pbrConstants.visibleSpotLights"_sh, lightCullingData.visibleSpotLightsBuffer);
			context.SetConstant("pbrConstants.directionalShadowMap"_sh, dirShadowData.shadowTexture);

			context.SetConstant("skyLight.irradiance"_sh, environmentTexturesData.irradiance);
			context.SetConstant("skyLight.radiance"_sh, environmentTexturesData.radiance);
			context.SetConstant("skyLight.lod"_sh, m_sceneEnvironment.lod);
			context.SetConstant("skyLight.intensity"_sh, m_sceneEnvironment.intensity);

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
			pipelineInfo.shader = ShaderMap::Get("FXAA");
			pipelineInfo.depthMode = RHI::DepthMode::None;
			auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

			context.BeginRendering(info);

			RCUtils::DrawFullscreenTriangle(context, pipeline, [&](RenderContext& context)
			{
				context.SetConstant("sceneColor"_sh, srcImage);
				context.SetConstant("viewData"_sh, uniformBuffers.viewDataBuffer);
				context.SetConstant("linearSampler"_sh, Renderer::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear>()->GetResourceHandle());
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
			pipelineInfo.shader = ShaderMap::Get("Tonemap");
			pipelineInfo.depthMode = RHI::DepthMode::None;
			auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

			context.BeginRendering(info);

			RCUtils::DrawFullscreenTriangle(context, pipeline, [&](RenderContext& context)
			{
				context.SetConstant("finalColor"_sh, srcImage);
				context.SetConstant("averageLuminance"_sh, averageLuminanceImage);
				context.SetConstant("middleGray"_sh, MiddleGray);
				context.SetConstant("whitePoint"_sh, WhitePoint * WhitePoint);
				context.SetConstant("frameIndex"_sh, m_frameIndex);

				BlueNoise::Setup(context, blueNoiseTextures);
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
				pipelineInfo.shader = ShaderMap::Get("VisualizationMeshShader");

				auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

				context.BeginRendering(info);
				context.BindPipeline(pipeline);

				context.SetConstant("visualizationMode"_sh, static_cast<uint32_t>(visualizationMode));

				SetupMeshPassConstants(context, blackboard);

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
				auto pipeline = ShaderMap::GetComputePipeline("VisualizationFullscreenShader");

				context.BindPipeline(pipeline);
				context.SetConstant("albedo"_sh, visualizationMode == VisualizationMode::BaseColor ? gBufferData.albedo : externalImages.white1x1);
				context.SetConstant("material"_sh, visualizationMode == VisualizationMode::Metallic || visualizationMode == VisualizationMode::Roughness ? gBufferData.material : externalImages.white1x1);
				context.SetConstant("sceneColor"_sh, visualizationMode == VisualizationMode::SceneColor ? shadingOutput.colorOutput : externalImages.white1x1);
				context.SetConstant("sceneDepth"_sh, visualizationMode == VisualizationMode::SceneDepth ? depthPrePass.depth : externalImages.white1x1);
				context.SetConstant("sceneNormal"_sh, visualizationMode == VisualizationMode::WorldNormal ? gBufferData.normals : externalImages.white1x1);
				context.SetConstant("sceneAO"_sh, visualizationMode == VisualizationMode::AmbientOcclusion ? gtaoOutput.outputImage : externalImages.white1x1);
				context.SetConstant("velocity"_sh, visualizationMode == VisualizationMode::Velocity ? depthPrePass.velocity : externalImages.white1x1);

				context.SetConstant("rwOutput"_sh, dstImage);
				context.SetConstant("renderSize"_sh, glm::uvec2(m_width, m_height));
				context.SetConstant("visualizationMode"_sh, static_cast<uint32_t>(visualizationMode));

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
			builder.ReadResource(uniformBuffers.directionalLightBuffer);

			GPUSceneData::SetupInputs(builder, gpuSceneData);
			BlueNoise::Build(builder, blueNoiseTextures);

			builder.SetIsRayTracingPass();
			builder.SetHasSideEffect();
		},
		[=](RenderContext& context)
		{
			RHI::RayTracingPipelineCreateInfo pipelineInfo;
			pipelineInfo.rayGenTable.emplace_back(ShaderMap::Get("RayGen"));
			pipelineInfo.missTable.emplace_back(ShaderMap::Get("Miss"));
			pipelineInfo.missTable.emplace_back(ShaderMap::Get("MissShadow"));
			pipelineInfo.closestHitTable.emplace_back(ShaderMap::Get("ClosestHit"));
			pipelineInfo.closestHitTable.emplace_back(ShaderMap::Get("ClosestHitShadow"));

			auto pipeline = ShaderMap::GetRayTracingPipeline(pipelineInfo);
			auto sbt = ShaderMap::GetShaderBindingTable(pipeline);

			context.BindPipeline(pipeline);

			GPUSceneData::SetupConstants(context, gpuSceneData);
			BlueNoise::Setup(context, blueNoiseTextures);

			context.SetConstant("viewData"_sh, uniformBuffers.viewDataBuffer);
			context.SetConstant("outputTexture"_sh, dstImage);
			context.SetConstant("frameIndex"_sh, m_frameIndex);
			context.SetConstant("directionalLight"_sh, uniformBuffers.directionalLightBuffer);
			context.SetAccelerationStructure(m_scene->GetRenderScene()->GetRayTracingScene()->GetAccelerationStructure());

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
