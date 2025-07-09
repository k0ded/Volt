#include "vrpch.h"
#include "Volt-Renderer/RenderingTechniques/CascadedDirectionalShadowTechnique.h"
#include "Volt-Renderer/Camera/Camera.h"
#include "Volt-Renderer/Mesh/MeshRenderer.h"
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
	struct CascadedDirectionalShadowVS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(CascadedDirectionalShadowVS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(CascadedDirectionalLightShadowMappingData, CascadedDirectionalLightShadowMapping)
			SHADER_PARAMETER_STRUCT_INCLUDE(GPUSceneParameters, GPUScene)
			SHADER_PARAMETER(uint, CascadeIndex)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(CascadedDirectionalShadowVS, "Engine/Shaders/Source/RenderPipelineLegacy/CascadedDirectionalShadowMap.hlsl", "MainVS", Vertex);

	struct CascadedDirectionalShadowPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(CascadedDirectionalShadowPS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(CascadedDirectionalShadowPS, "Engine/Shaders/Source/RenderPipelineLegacy/CascadedDirectionalShadowMap.hlsl", "MainPS", Pixel);

	BEGIN_SHADER_PARAMETER_STRUCT(CascadedDirectionalShadowParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(CascadedDirectionalShadowVS::Parameters, VS)
		SHADER_PARAMETER_STRUCT_INCLUDE(CascadedDirectionalShadowPS::Parameters, PS)
	END_SHADER_PARAMETER_STRUCT()

	CascadedDirectionalShadowTechnique::CascadedDirectionalShadowTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
		: m_renderGraph(renderGraph),
		m_blackboard(blackboard)
	{

	}

	CascadedDirectionalShadowTechnique::Result CascadedDirectionalShadowTechnique::Execute(const RenderView& view, const RenderLightData& renderLightData)
	{
		constexpr float Size = 1000.f;

		Ref<Camera> shadowCamera = CreateRef<Camera>(-Size, Size, -Size, Size, 1.f, 100'000.f);
		const glm::mat4 viewMatrix = glm::lookAt(renderLightData.description.direction * 1000.f, glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
		shadowCamera->SetView(viewMatrix);
	
		m_renderGraph.BeginMarker("CascadedDirectionalShadow");

		RGUniformBufferRef directionalLightUniformBuffer = UploadUniformBufferData(view, renderLightData);

		RGTextureDesc directionalShadowTextureDesc{};
		directionalShadowTextureDesc.format = RHI::PixelFormat::D32_SFLOAT;
		directionalShadowTextureDesc.width = 2048;
		directionalShadowTextureDesc.height = 2048;
		directionalShadowTextureDesc.usage = RHI::ImageUsage::Attachment;
		directionalShadowTextureDesc.layers = DirectionalLightShadowUniformBuffer::CASCADE_COUNT;
		directionalShadowTextureDesc.debugName = "CascadedDirectionalLightShadowMap";

		RGTextureRef directionalShadowTexture = m_renderGraph.CreateTexture(directionalShadowTextureDesc);

		auto vertexShader = ShaderMap::Get<CascadedDirectionalShadowVS>();
		auto pixelShader = ShaderMap::Get<CascadedDirectionalShadowPS>();

		RHI::RenderPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.depthCompareOperator = RHI::CompareOperator::LessEqual;

		for (uint32_t i = 0; i < DirectionalLightShadowUniformBuffer::CASCADE_COUNT; ++i)
		{
			CullingInfo cullingInfo{};
			cullingInfo.type = CullingInfo::Type::Orthographic;
			cullingInfo.nearPlane = shadowCamera->GetNearPlane();
			cullingInfo.farPlane = shadowCamera->GetFarPlane();
			cullingInfo.cullingFrustum = shadowCamera->GetFrustumCullingInfo();
			cullingInfo.viewMatrix = shadowCamera->GetView();

			MeshRenderer meshRenderer;
			meshRenderer.BuildRenderCommands(m_renderGraph, view.renderScene, cullingInfo, vertexShader, pixelShader, pipelineCreateInfo);

			CascadedDirectionalShadowParameters* passParameters = m_renderGraph.AllocParameters<CascadedDirectionalShadowParameters>();
			passParameters->VS.CascadedDirectionalLightShadowMapping = directionalLightUniformBuffer;
			passParameters->VS.GPUScene = view.renderScene->GetGPUSceneParameters(m_renderGraph);
			passParameters->VS.CascadeIndex = i;
			passParameters->PS.renderTargets.depthTarget = directionalShadowTexture;

			const std::string passName = std::format("CascadedDirectionalShadow Cascade: {}", i);

			m_renderGraph.AddPass(passName,
				RenderGraphPassFlags::None,
				passParameters,
				[passParameters, view, meshRenderer, i](RenderContext& context) 
			{
				BatchedShaderParameters batchedShaderParameters;
				context.CollectParameters(passParameters, batchedShaderParameters);

				RenderingInfo renderingInfo = context.CreateRenderingInfo(2048, 2048, passParameters->PS.renderTargets);
				renderingInfo.renderingInfo.layerCount = DirectionalLightShadowUniformBuffer::CASCADE_COUNT;
				renderingInfo.renderingInfo.depthAttachmentInfo.SetClearColor(1.f, 1.f, 1.f, 1.f);

				if (i > 0)
				{
					renderingInfo.renderingInfo.depthAttachmentInfo.clearMode = RHI::ClearMode::Load;
				}

				context.BeginRendering(renderingInfo);
				meshRenderer.Render(context, batchedShaderParameters);
				context.EndRendering();
			});
		}

		m_renderGraph.EndMarker();
	
		return { directionalShadowTexture, directionalLightUniformBuffer };
	}

	RGUniformBufferRef CascadedDirectionalShadowTechnique::UploadUniformBufferData(const RenderView& view, const RenderLightData& renderLightData)
	{
		RGUniformBufferRef uniformBuffer = m_renderGraph.CreateUniformBuffer(RGUniformBufferDesc::Create<DirectionalLightShadowUniformBuffer>("DirectionalLightUniformBuffer"));

		DirectionalLightShadowUniformBuffer lightInfo;

		if (renderLightData.description.castShadows)
		{
			const Vector<float> cascades = { view.camera->GetFarPlane() / 50.f, view.camera->GetFarPlane() / 25.f, view.camera->GetFarPlane() / 10.f };
			const auto lightMatrices = Utility::CalculateCascadeMatrices(view.camera, renderLightData.description.direction, cascades);

			for (size_t i = 0; i < lightMatrices.size(); i++)
			{
				const auto& matrices = lightMatrices.at(i);
				lightInfo.viewProjections[i] = matrices.projection * matrices.view;
			}

			for (size_t i = 0; i < cascades.size() + 1; i++)
			{
				if (i < cascades.size())
				{
					lightInfo.cascadeDistances[i] = cascades.at(i);
				}
				else
				{
					lightInfo.cascadeDistances[i] = view.camera->GetFarPlane();
				}
			}
		}

		AddMappedBufferUpload(m_renderGraph, uniformBuffer, &lightInfo, sizeof(DirectionalLightShadowUniformBuffer));

		return uniformBuffer;
	}
}
