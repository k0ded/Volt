#include "vrpch.h"
#include "Volt-Renderer/RenderingTechniques/CascadedShadowMapsTechnique.h"
#include "Volt-Renderer/MeshPassProcessors/CascadedShadowMapsMeshProcessor.h"

#include "Volt-Renderer/Camera/Camera.h"
#include "Volt-Renderer/RenderPrimitiveData.h"
#include "Volt-Renderer/RendererCommon.h"
#include "Volt-Renderer/RenderView.h"
#include "Volt-Renderer/ShadowMappingUtility.h"
#include "Volt-Renderer/GPUScene.h"
#include "Volt-Renderer/RenderScene.h"

#include <RenderCore/RenderGraph/ShaderRegistry.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderContext.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/BatchedShaderParameters.h>

namespace Volt
{
	VT_REGISTER_SHADER(CascadedDirectionalShadowVS, "Engine/Shaders/Source/RenderPipelineLegacy/CascadedDirectionalShadowMap.hlsl", "MainVS", Vertex);
	VT_REGISTER_SHADER(CascadedDirectionalShadowPS, "Engine/Shaders/Source/RenderPipelineLegacy/CascadedDirectionalShadowMap.hlsl", "MainPS", Pixel);

	BEGIN_SHADER_PARAMETER_STRUCT(CascadedDirectionalShadowParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(CascadedDirectionalShadowVS::Parameters, VS)
		SHADER_PARAMETER_STRUCT_INCLUDE(CascadedDirectionalShadowPS::Parameters, PS)
		SHADER_PARAMETER_STRUCT_INCLUDE(MeshPassProcessorParameters, ProcessorParameters)
	END_SHADER_PARAMETER_STRUCT()

	CascadedShadowMapsTechnique::CascadedShadowMapsTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, CascadedShadowMapMeshProcessor* meshProcessor)
		: m_renderGraph(renderGraph),
		m_blackboard(blackboard),
		m_meshProcessor(meshProcessor)
	{

	}

	CascadedShadowMapsTechnique::Result CascadedShadowMapsTechnique::Execute(const RenderView& view, const RenderLightData& renderLightData)
	{
		constexpr float Size = 1000.f;

		Ref<Camera> shadowCamera = CreateRef<Camera>(-Size, Size, -Size, Size, 1.f, 100'000.f);
		const glm::mat4 viewMatrix = glm::lookAt(renderLightData.description.direction * 1000.f, glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
		shadowCamera->SetView(viewMatrix);
	
		m_renderGraph.BeginMarker("CascadedDirectionalShadow");

		RGUniformBufferRef directionalLightUniformBuffer = UploadUniformBufferData(view, renderLightData);

		GenerateCascades(view, renderLightData);

		RGTextureDesc directionalShadowTextureDesc{};
		directionalShadowTextureDesc.format = RHI::PixelFormat::D32_SFLOAT;
		directionalShadowTextureDesc.width = 2048;
		directionalShadowTextureDesc.height = 2048;
		directionalShadowTextureDesc.usage = RHI::ImageUsage::Attachment;
		directionalShadowTextureDesc.layers = DirectionalLightShadowUniformBuffer::NumCascades;
		directionalShadowTextureDesc.debugName = "CascadedDirectionalLightShadowMap";

		RGTextureRef directionalShadowTexture = m_renderGraph.CreateTexture(directionalShadowTextureDesc);

		m_meshProcessor->PrepareRenderCommands(m_renderGraph);

		for (uint32_t i = 0; i < DirectionalLightShadowUniformBuffer::NumCascades; ++i)
		{
			CascadedDirectionalShadowParameters* passParameters = m_renderGraph.AllocParameters<CascadedDirectionalShadowParameters>();
			passParameters->VS.CascadedDirectionalLightShadowMapping = directionalLightUniformBuffer;
			passParameters->VS.GPUScene = view.renderScene->GetGPUSceneParameters(m_renderGraph);
			passParameters->VS.CascadeIndex = i;
			passParameters->PS.renderTargets.depthTarget = directionalShadowTexture;
			passParameters->ProcessorParameters = m_meshProcessor->GetParameters(m_renderGraph);

			const std::string passName = std::format("CascadedDirectionalShadow Cascade: {}", i);

			m_renderGraph.AddPass(passName,
				RenderGraphPassFlags::Raster,
				passParameters,
				[passParameters, view, meshPassProcessor = m_meshProcessor, i](RenderContext& context) 
			{
				BatchedShaderParameters batchedShaderParameters;
				context.CollectParameters(passParameters, batchedShaderParameters);

				RenderingInfo renderingInfo = context.CreateRenderingInfo(2048, 2048, passParameters->PS.renderTargets);
				renderingInfo.renderingInfo.layerCount = DirectionalLightShadowUniformBuffer::NumCascades;
				renderingInfo.renderingInfo.depthAttachmentInfo.SetClearColor(1.f, 1.f, 1.f, 1.f);

				if (i > 0)
				{
					renderingInfo.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;
				}

				context.BeginRendering(renderingInfo);
				meshPassProcessor->ExecuteCommands(context, batchedShaderParameters);
				context.EndRendering();
			});
		}

		m_renderGraph.EndMarker();
	
		return { directionalShadowTexture, directionalLightUniformBuffer };
	}

	std::pair<glm::mat4, glm::mat4> GenerateCascadeMatrix(const RenderView& view, const RenderLightData& renderLightData, float startDistance, float endDistance, uint32_t resolution)
	{
		const float viewFieldOfView = view.camera->GetFieldOfView();
		const float viewAspectRatio = view.camera->GetAspectRatio();
		const float invG = glm::tan(viewFieldOfView * 0.5f);

		Array<glm::vec3, 8> cascadeFrustumCorners;

		cascadeFrustumCorners[0] = glm::vec3((startDistance * viewAspectRatio) * invG, -(startDistance * invG), startDistance);
		cascadeFrustumCorners[1] = glm::vec3((startDistance * viewAspectRatio) * invG, (startDistance * invG), startDistance);
		cascadeFrustumCorners[2] = glm::vec3(-(startDistance * viewAspectRatio) * invG, (startDistance * invG), startDistance);
		cascadeFrustumCorners[3] = glm::vec3(-(startDistance * viewAspectRatio) * invG, -(startDistance * invG), startDistance);

		cascadeFrustumCorners[4] = glm::vec3((endDistance * viewAspectRatio) * invG, -(endDistance * invG), endDistance);
		cascadeFrustumCorners[5] = glm::vec3((endDistance * viewAspectRatio) * invG, (endDistance * invG), endDistance);
		cascadeFrustumCorners[6] = glm::vec3(-(endDistance * viewAspectRatio) * invG, (endDistance * invG), endDistance);
		cascadeFrustumCorners[7] = glm::vec3(-(endDistance * viewAspectRatio) * invG, -(endDistance * invG), endDistance);

		glm::mat4 lightObjectToWorld = glm::identity<glm::mat4>();
		{
			glm::vec3 zAxis = -renderLightData.description.direction;
			glm::vec3 up = glm::vec3(0, 1, 0);
			if (fabs(glm::dot(up, zAxis)) > 0.9f)
			{
				up = glm::vec3(1, 0, 0);
			}

			glm::vec3 xAxis = glm::normalize(glm::cross(up, zAxis));
			glm::vec3 yAxis = glm::cross(zAxis, xAxis);
		
			lightObjectToWorld[0] = glm::vec4(xAxis, 0.f);
			lightObjectToWorld[1] = glm::vec4(yAxis, 0.f);
			lightObjectToWorld[2] = glm::vec4(zAxis, 0.f);
		}

		const glm::mat4 viewTransform = view.camera->GetTransform();

		const glm::mat4 transformMat = glm::inverse(lightObjectToWorld) * viewTransform;
		const float projectionSize = glm::ceil(glm::max(glm::length(cascadeFrustumCorners[0] - cascadeFrustumCorners[6]), glm::length(cascadeFrustumCorners[4] - cascadeFrustumCorners[6])));

		for (glm::vec3& p : cascadeFrustumCorners)
		{
			p = glm::vec3(transformMat * glm::vec4(p, 1.f));
		}

		glm::vec3 min = std::numeric_limits<float>::max();
		glm::vec3 max = -std::numeric_limits<float>::max();

		for (const glm::vec3& p : cascadeFrustumCorners)
		{
			min.x = glm::min(min.x, p.x);
			min.y = glm::min(min.y, p.y);
			min.z = glm::min(min.z, p.z);

			max.x = glm::max(max.x, p.x);
			max.y = glm::max(max.y, p.y);
			max.z = glm::max(max.z, p.z);
		}

		const float texelSize = projectionSize / float(resolution);
		glm::vec3 center = glm::vec3(glm::ceil((max.x + min.x) / (2.f * texelSize)) * texelSize, glm::ceil((max.y + min.y) / (2.f * texelSize)) * texelSize, min.z);

		glm::mat4 lightObjectToWorldTransposed = glm::transpose(lightObjectToWorld);

		glm::mat4 cascadeToWorld = glm::identity<glm::mat4>();
		cascadeToWorld[0] = lightObjectToWorldTransposed[0];
		cascadeToWorld[1] = lightObjectToWorldTransposed[1];
		cascadeToWorld[2] = lightObjectToWorldTransposed[2];
		cascadeToWorld[3] = glm::vec4(-center, 1.f);

		glm::mat4 projectionMatrix = glm::identity<glm::mat4>();
		projectionMatrix[0][0] = 2.f / projectionSize;
		projectionMatrix[1][1] = 2.f / projectionSize;
		projectionMatrix[2][2] = 1.f / (max.z - min.z);
		projectionMatrix[2][3] = 0.f;

		glm::mat4 viewProjection = projectionMatrix * cascadeToWorld;

		glm::mat4 shadowProjectionMatrix = glm::identity<glm::mat4>();
		shadowProjectionMatrix[0][0] = 1.f / projectionSize;
		shadowProjectionMatrix[1][1] = 1.f / projectionSize;
		shadowProjectionMatrix[2][2] = 1.f / (max.z - min.z);
		shadowProjectionMatrix[3][0] = 0.5f;
		shadowProjectionMatrix[3][1] = 0.5f;

		glm::mat4 shadowMatrix = shadowProjectionMatrix * cascadeToWorld;

		return { viewProjection, shadowMatrix };
	}

	RGUniformBufferRef CascadedShadowMapsTechnique::UploadUniformBufferData(const RenderView& view, const RenderLightData& renderLightData)
	{
		RGUniformBufferRef uniformBuffer = m_renderGraph.CreateUniformBuffer(RGUniformBufferDesc::Create<DirectionalLightShadowUniformBuffer>("DirectionalLightUniformBuffer"));

		DirectionalLightShadowUniformBuffer lightInfo;

		constexpr float MaxShadowDistance = 32'000;

		const Array<glm::vec2, DirectionalLightShadowUniformBuffer::NumCascades> CascadeDistances = {
			glm::vec2{ view.camera->GetNearPlane(), 1500.f },
			glm::vec2{ 1000.f, 5000.f },
			glm::vec2{ 4000.f, 12'000.f },
			glm::vec2{ 10'000.f, MaxShadowDistance }
		};

		if (renderLightData.description.castShadows)
		{
			const uint32_t shadowMapResolution = 2048u;

			for (uint32_t i = 0; i < DirectionalLightShadowUniformBuffer::NumCascades; ++i)
			{
				const glm::vec2& cascadeDistances = CascadeDistances[i];

				auto [viewProjection, shadowMatrix] = GenerateCascadeMatrix(view, renderLightData, cascadeDistances.x, cascadeDistances.y, shadowMapResolution);

				lightInfo.viewProjections[i] = viewProjection;
				lightInfo.cascadeDistances[i] = glm::vec4(cascadeDistances.x, cascadeDistances.y, 0.f, 0.f);

				if (i == 0)
				{
					lightInfo.cascade0Matrix = shadowMatrix;
				}
				else
				{
					glm::mat4 cascadeTransformMat = shadowMatrix * glm::inverse(lightInfo.cascade0Matrix);
					lightInfo.cascadeScale[i - 1] = glm::vec4{cascadeTransformMat[0][0], cascadeTransformMat[1][1], cascadeTransformMat[2][2], 0.f };
					lightInfo.cascadeOffset[i - 1] = cascadeTransformMat[3];
				}
			}

			// Setup sampling offsets
			{
				const float invResolution = 1.f / float(shadowMapResolution);
				const float delta = (3.f / 16.f) * invResolution;

				lightInfo.samplingOffsets[0] = glm::vec4(-delta, -3.f * delta, 3.f * delta, -delta);
				lightInfo.samplingOffsets[1] = glm::vec4(delta, 3.f * delta, -3.f * delta, delta);
			}
		}

		AddMappedBufferUploadCopyData(m_renderGraph, uniformBuffer, &lightInfo, sizeof(DirectionalLightShadowUniformBuffer));

		return uniformBuffer;
	}

	RGUniformBufferRef CascadedShadowMapsTechnique::GenerateCascades(const RenderView& view, const RenderLightData& renderLightData)
	{
		const float viewNearPlane = view.camera->GetNearPlane();
		//const float viewFarPlane = view.camera->GetFarPlane();
		const float viewFieldOfView = view.camera->GetFieldOfView();
		const float viewAspectRatio = view.camera->GetAspectRatio();
		const float invG = glm::tan(viewFieldOfView * 0.5f);

		const float cascadeEnd = 1000.f;

		Array<glm::vec3, 8> cascadeFrustumCorners;

		cascadeFrustumCorners[0] = glm::vec3((viewNearPlane * viewAspectRatio) * invG, -(viewNearPlane * invG), viewNearPlane);
		cascadeFrustumCorners[1] = glm::vec3((viewNearPlane * viewAspectRatio) * invG, (viewNearPlane * invG), viewNearPlane);
		cascadeFrustumCorners[2] = glm::vec3(-(viewNearPlane * viewAspectRatio) * invG, (viewNearPlane * invG), viewNearPlane);
		cascadeFrustumCorners[3] = glm::vec3(-(viewNearPlane * viewAspectRatio) * invG, -(viewNearPlane * invG), viewNearPlane);

		cascadeFrustumCorners[4] = glm::vec3((cascadeEnd * viewAspectRatio) * invG, -(cascadeEnd * invG), cascadeEnd);
		cascadeFrustumCorners[5] = glm::vec3((cascadeEnd * viewAspectRatio) * invG, (cascadeEnd * invG), cascadeEnd);
		cascadeFrustumCorners[6] = glm::vec3(-(cascadeEnd * viewAspectRatio) * invG, (cascadeEnd * invG), cascadeEnd);
		cascadeFrustumCorners[7] = glm::vec3(-(cascadeEnd * viewAspectRatio) * invG, -(cascadeEnd * invG), cascadeEnd);

		const glm::mat4 viewTransform = view.camera->GetTransform();
		const glm::mat4 lightView = glm::mat4_cast(glm::quatLookAt(renderLightData.description.direction, glm::vec3{ 0.f, 1.f, 0.f }));

		const glm::mat4 transformMat = viewTransform * glm::inverse(lightView);

		for (glm::vec3& p : cascadeFrustumCorners)
		{
			p = glm::vec3(transformMat * glm::vec4(p, 1.f));
		}

		glm::vec3 min = std::numeric_limits<float>::max();
		glm::vec3 max = -std::numeric_limits<float>::max();

		for (const glm::vec3& p : cascadeFrustumCorners)
		{
			min.x = glm::min(min.x, p.x);
			min.y = glm::min(min.y, p.y);
			min.z = glm::min(min.z, p.z);

			max.x = glm::max(max.x, p.x);
			max.y = glm::max(max.y, p.y);
			max.z = glm::max(max.z, p.z);
		}

		glm::vec3 center = glm::vec3((max.x + min.x) * 0.5f, (max.y + min.y) * 0.5f, min.z);
		center = glm::vec3(viewTransform * glm::vec4(center, 1.f));

		const float projectionSize = glm::max(max.x - min.x, max.y - min.y);

		const glm::mat4 viewMatrix = glm::lookAt(renderLightData.description.direction + center, center, { 0.f, 1.f, 0.f });
		
		glm::mat4 projection = glm::identity<glm::mat4>();
		projection[0][0] = 2.f / projectionSize;
		projection[1][1] = 2.f / projectionSize;
		projection[2][2] = 1.f / (max.z - min.z);

		return nullptr;
	}
}
